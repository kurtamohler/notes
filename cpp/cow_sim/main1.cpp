#include <memory>
#include <iostream>
#include <variant>

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

// Note: This example doesn't yet include the StorageImpl side
// of the COW simulation. That will likely be implemented in either
// of the following ways:
//
//  * Somehow add an optional layer of indirection for the StorageImpl
//    if it needs to simulate COW.
//
//  * Add a COW simulation state machine as a member of StorageImpl,
//    like https://github.com/pytorch/pytorch/pull/97175
class StorageImpl {
 public:
  const void* data() const {
    return data_ptr_.get();
  }

  void* mutable_data() {
    return data_ptr_.mutable_get();
  }

 private:
  DataPtr data_ptr_;
};

// This class is optionally inserted as an extra layer of indirection between a
// `Storage` and the `StorageImpl` that it points to. It handles keeping track
// of would-be outputs of a lazy clone operation. So it will have the same
// purpose as the `ShadowStorageMixin` that `TensorImpl` inherits in
// https://github.com/pytorch/pytorch/pull/97175. But the advantage of this
// implementation is that we don't need to add any new members to `TensorImpl`
// or `Storage`. But we do have to change `Storage`'s
// `intrusive_ptr<StorageImpl> storage_impl_` to be a variant between
// `intrusive_ptr<StorageImpl>` and `intrusive_ptr<StorageCOWSim>`.
// And because of that, we need to use visitors everywhere `storage_impl_` is
// accessed in the implementation of `Storage`.
class StorageCOWSim {
 public:
  StorageCOWSim(std::shared_ptr<StorageImpl> storage_impl)
    : storage_impl_(storage_impl) {}

  const void* data() const {
    return storage_impl_->data();
  }

  void* mutable_data() {
    maybe_simulate_cow_materialize();
    return storage_impl_->mutable_data();
  }

 private:

  void maybe_simulate_cow_materialize() {
    std::cout << "maybe simulating materialize" << std::endl;
  }

  // Note that PyTorch uses `intrusive_ptr`, but we're using `std::shared_ptr`
  // in this example just to keep it lightweight.
  std::shared_ptr<StorageImpl> storage_impl_;
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// This function is taken from: https://gist.github.com/s3rvac/d1f30364ce1f732d75ef0c89a1c8c1ef
template<typename Variant, typename... Alternatives>
decltype(auto) visit_variant(Variant&& variant, Alternatives&&... alternatives) {
  return std::visit(
    overloaded{std::forward<Alternatives>(alternatives)...},
    std::forward<Variant>(variant)
  );
}

using storage_impl_ptr_variant_t = std::variant<
  std::shared_ptr<StorageImpl>,
  std::shared_ptr<StorageCOWSim>>;

class Storage {
 public:
  Storage() : storage_impl_(std::make_shared<StorageImpl>()) {}

  const void* data() const {
    return visit_variant(storage_impl_,
      [](std::shared_ptr<StorageImpl> storage_impl) {
        std::cout << "only StorageImpl" << std::endl;
        return storage_impl->data();
      },
      [](std::shared_ptr<StorageCOWSim> storage_impl) {
        std::cout << "found StorageCOWSim" << std::endl;
        return storage_impl->data();
      }
    );
  }

  void* mutable_data() const {
    return visit_variant(storage_impl_,
      [](std::shared_ptr<StorageImpl> storage_impl) {
        std::cout << "only StorageImpl" << std::endl;
        return storage_impl->mutable_data();
      },
      [](std::shared_ptr<StorageCOWSim> storage_impl) {
        std::cout << "found StorageCOWSim" << std::endl;
        return storage_impl->mutable_data();
      }
    );
  }

  // Adds an extra layer of indirection between `Storage` and `StorageImpl` to
  // simulate COW behavior. No-op if the layer has already been added.
  void maybe_enable_cow_sim_() {
    visit_variant(storage_impl_,
      [&](std::shared_ptr<StorageImpl> storage_impl) {
        std::cout << "enabling COW simulation" << std::endl;
        storage_impl_ = std::make_shared<StorageCOWSim>(storage_impl);
      },
      [](std::shared_ptr<StorageCOWSim>) {}
    );
  }

 private:
  storage_impl_ptr_variant_t storage_impl_;
};


class TensorImpl {
 public:
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

  void maybe_enable_cow_sim_() {
    storage_.maybe_enable_cow_sim_();
  }

 private:
  template <typename Void, typename Func>
  Void* data_impl(const Func& get_data) const {
    // Note: PyTorch does some checks in here
    auto* data = get_data();
    return data;
  }

  Storage storage_;
};

int main() {
  TensorImpl a;
  TensorImpl b;

  std::cout << "------------------------" << std::endl;
  b.maybe_enable_cow_sim_();
  b.maybe_enable_cow_sim_();

  std::cout << "------------------------" << std::endl;
  std::cout << a.const_data() << std::endl;
  std::cout << "------------------------" << std::endl;
  std::cout << b.const_data() << std::endl;
  std::cout << "------------------------" << std::endl;
  std::cout << a.mutable_data() << std::endl;
  std::cout << "------------------------" << std::endl;
  std::cout << b.mutable_data() << std::endl;
}