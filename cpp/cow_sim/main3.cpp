#include <functional>
#include <ios>
#include <memory>
#include <iostream>
#include <optional>
#include <variant>


// Note: This example doesn't yet include the StorageImpl side
// of the COW simulation. That will likely be implemented in either
// of the following ways:
//
//  * Somehow add an optional layer of indirection for the StorageImpl
//    if it needs to simulate COW.
//
//  * Add a COW simulation state machine as a member of StorageImpl,
//    like https://github.com/pytorch/pytorch/pull/97175
namespace COWSim {
struct WarnCounter {
  uint8_t count = 0;
  void increment() { ++count; }
  void reset() { count =0; }
};

WarnCounter &get_warn_counter() {
  static WarnCounter warn_counter;
  return warn_counter;
}

void reset_warn_counter() {
  get_warn_counter().reset();
}

void check_warn_counter(uint8_t expected_count) {
  if (expected_count == get_warn_counter().count) {
    std::cout << "yay.\n";
  } else {
    std::cout << "BOO.\n";
  }
}

using TokenType = std::uintptr_t;
//This could also populate a shared registry with extra info about "who" this
//token is associated with that can be recalled at message construction time.
//That would introduce some thread safety issues, but is an option if the
//additional context is needed
template <typename T> inline TokenType mint_token_for(T *addr) {
  // In this simple example I have used a type alias for the token type and
  // chosen as sentinel value the token created for NULL. In practice a stronger
  // token type and singleton sentinel value would be safer
  //TORCH_INTERNAL_ASSERT(addr != nullptr) 
  return reinterpret_cast<TokenType>(addr);
}
//Note that mint_token_for(nullptr) can't be constexpr if that is important.
static constexpr TokenType NullToken{0};
static const char cow_read_msg[] = "WRITE THEN READ";
static const char cow_write_msg[] = "WRITE THEN WRITE";

inline bool token_set(TokenType t) { return t != NullToken;}

struct COWChecker {
  //Might be useful,
  TokenType creator_;
  //Probably more than one field, not a string until msg generation
  std::string note_;
  //The only piece of data critical to the logic
  TokenType first_writer_;
  COWChecker() : creator_(NullToken), note_("<info>"), first_writer_(NullToken) {}
  explicit COWChecker(TokenType createdBy) : creator_(createdBy), note_("<info>"), first_writer_(NullToken) {}

  // In practice these methods would craft a real message using note_ to hint at
  // where the COW behavior would kick in. Some boilerplate tacked on for docs
  // links, or how to opt in to COW ahead of time for testing
  void check_on_write(TokenType writer) {
    maybe_warn_on_mismatch(writer, cow_write_msg);
    maybe_set_first_writer(writer);
  }
  void check_on_read(TokenType reader) const {
    maybe_warn_on_mismatch(reader, cow_read_msg);
  }

  void maybe_warn_on_mismatch(TokenType other, const char *msg) const {
    // check if the cow state has been initalized
    if (token_set(creator_)) {
      //Check that we have been subsequently hit with a reason to materialize
      if (token_set(first_writer_))
        // issue warning
        if (first_writer_ != other) {
          get_warn_counter().increment();
          std::cout << "COW BEHAVIOR WARNING: " << msg << std::endl;
        }
    }
  }

  void maybe_set_first_writer(TokenType other) {
    //We if cow has been initalized
    if (token_set(creator_)) {
      //We are setting the first writer
      if (!token_set(first_writer_)) {
        first_writer_ = other;
      }
    }
  }

  void maybe_init(TokenType creator) {
    if(!token_set(creator_)) creator_ = creator;
  }

};

} // namespace COWSim

static int64_t next_data_ptr_value_ = 1;

class DataPtr {
 public:
  DataPtr() : ptr_(reinterpret_cast<void*>(next_data_ptr_value_++)) {}

  void* get() const {
    return ptr_;
  }

  void* mutable_get() {
    return ptr_;
  }

 private:
  void* ptr_;
};

class StorageImpl {
 public:
   const void *data(COWSim::TokenType token) const {
    cow_checker_.check_on_read(token);
    return data_ptr_.get();
  }

  void *mutable_data(COWSim::TokenType token) {
    cow_checker_.check_on_write(token);
    return data_ptr_.mutable_get();
  }

  void maybe_enable_cow_sim(COWSim::TokenType token) {
    // Note tracking the creator may be useful for generating informative
    // messages. here I am using to signal cow initialization, but that can be
    // done other ways without expanding the scope of impact. Optionals for
    // example would work.
    // Here I have purposely pushed as much logic down into the checker as I
    // can, so that the diff footprint on Storage/StorageImpl is as small as
    // possible.
    cow_checker_.maybe_init(token);
  }

 private:
   DataPtr data_ptr_;
   COWSim::COWChecker cow_checker_;
};


class Storage {
 public:
   Storage() : storage_impl_(std::make_shared<StorageImpl>()), group_number_(1) {}
   Storage(Storage& other) : storage_impl_(other.storage_impl_), group_number_(other.group_number_) {}

  const void *data() const {
    return storage_impl_->data(reinterpret_cast<std::uintptr_t>(group_number_));
  }

  void *mutable_data() const {
    return storage_impl_->mutable_data(reinterpret_cast<std::uintptr_t>(group_number_));
  }

  // Adds an extra layer of indirection between `Storage` and `StorageImpl` to
  // simulate COW behavior. No-op if the layer has already been added.
  void maybe_enable_cow_sim_() {
    // Note if we want to hoist the creator token (it will be unique to the
    // tensorimpl rather than the Storage) This API would need to accept it as
    // an arg.  Since storages are uniquely owned wrappers sharing StorageImpl s
    // This is probably okay
    storage_impl_->maybe_enable_cow_sim(group_number_);
  }

  void increment_group_number_() {
    group_number_++;
  }

 private:
  std::shared_ptr<StorageImpl> storage_impl_;
  COWSim::TokenType group_number_;
};

//Except for differences used to minify the example there are no COWSim details leaking into TensorImpl
class TensorImpl {
public:
  TensorImpl() = default;
  TensorImpl(Storage storage) : storage_(storage) {}
  Storage& storage() {
    return storage_;
  }

  inline const void* const_data() const {
    return data_impl<const void>(
      [this] { return static_cast<const char*>(storage_.data()); });
  }

  inline void* mutable_data() {
    return data_impl<void>(
      [this] { return static_cast<char*>(storage_.mutable_data()); });
  }

  // void maybe_enable_cow_sim_() {
  //   storage_.maybe_enable_cow_sim_();
  // }


 private:
  template <typename Void, typename Func>
  Void* data_impl(const Func& get_data) const {
    // Note: PyTorch does some checks in here
    auto* data = get_data();
    return data;
  }

  Storage storage_;
};

namespace torch {
// Clone/View defined to refer to them, they are not modified
TensorImpl clone(TensorImpl self) { return TensorImpl(); }
TensorImpl view(TensorImpl self) { return TensorImpl(self.storage());}

enum ReshapeArgs { View, Copy };
// Simulating changes to the impl of an op w/ conditional view behavior
TensorImpl reshape(TensorImpl self, ReshapeArgs args) {
  if (args == ReshapeArgs::View) {
    // this would be added inside the branch where we are creating a view e.g
    // https://github.com/pytorch/pytorch/blob/12a69afa6d4c4e9720d1b7644291eadb49792e8a/aten/src/ATen/native/TensorShape.cpp#L1670
    self.storage().maybe_enable_cow_sim_();
    TensorImpl res = view(self);
    res.storage().increment_group_number_();
    return res;
  } else {
    return clone(self);
  }
}
}

// for easy to read examples
auto tensor() { return TensorImpl(); }
using torch::view;
using torch::reshape;
auto mutates_input(TensorImpl& t) { return t.mutable_data(); }
auto reads_from_input(TensorImpl& t) {return t.const_data();}
using torch::ReshapeArgs;

auto example_1() {
  auto a = tensor();
  auto b = reshape(a, ReshapeArgs::View);
  mutates_input(a);
  reads_from_input(b);
}

auto example_2() {
  auto a = tensor();
  auto c = tensor();
  c.storage() = a.storage();
  auto b = reshape(a, ReshapeArgs::View);
  mutates_input(b);
  reads_from_input(c);
}

auto example_3() {
    TensorImpl a;
    TensorImpl b = view(a);
    mutates_input(a);
    mutates_input(b);
    reads_from_input(a);
    reads_from_input(b);
}

auto example_4() {
  auto a = tensor();
  auto b = view(a);
  auto c = reshape(b, ReshapeArgs::View);
  mutates_input(b);
  reads_from_input(a);
}

auto example_5() {
  auto a = tensor();
  auto b = reshape(a, ReshapeArgs::View);
  auto c = view(b);
  mutates_input(b);
  reads_from_input(c);
}

auto example_6() {
  auto a = tensor();
  auto b = reshape(a, ReshapeArgs::View);
  auto c = view(b);
  mutates_input(c);
  reads_from_input(b);
}

auto example_7() {
  auto a = tensor();
  auto b = view(a);
  auto c = reshape(b, ReshapeArgs::View);
  mutates_input(a);
  reads_from_input(b);
}

using func_t = decltype(&example_1);
static std::pair<func_t, uint8_t> test_cases[] = {
    {&example_1, 1},
    {&example_2, 1},
    {&example_3, 0},
    {&example_4, 0},
    {&example_5, 0},
    {&example_6, 0},
    {&example_7, 0},
};



int main() {
  auto i = 0;
  for (auto [func, expected_warns] : test_cases) {
    ++i;
    COWSim::get_warn_counter().reset();
    func();
    std::cout << "Example " << i << "-->";
    COWSim::check_warn_counter(expected_warns);
  }
}