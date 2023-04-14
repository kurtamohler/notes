// We could potentially implement a simulated shared pointer within
// `UniqueVoidPtr` by taking advantage of its `void* ctx_` member. We could
// store an object in there which holds onto the refcount, a pointer to the
// managed data buffer, and a pointer to the concrete (device-specific) deleter
// function to call when the refcount goes to 0.
//
// In this example, we don't need to modify how `UniqueVoidPtr` works. We can
// continue using `DeleterFnPtr = void (*)(void*)`. We wouldn't be changing
// it a functional object for the deleter. I'm not sure if that's really a good
// thing or not though, since this example isn't very type safe.

#include <memory>
#include <iostream>

using DeleterFnPtr = void (*)(void*);

void example_cpu_deleter(void* ctx) {
  std::cout << "in example_cpu_dealloc" << std::endl;
  void* ptr = ctx;
  std::cout << "  ptr: " << ptr << std::endl;
}

struct RefcountedDeleterContext {
  RefcountedDeleterContext(void* ptr, DeleterFnPtr concrete_deleter)
    : ptr(ptr), concrete_deleter(concrete_deleter), refcount(1) {}

  ~RefcountedDeleterContext() {
    std::cout << "in RefcountedDeleterContext::~RefcountedDeleterContext" << std::endl;
  }

  void incref() {
    refcount++;
  }

  DeleterFnPtr concrete_deleter;
  size_t refcount;
  void* ptr;
};

void refcounted_deleter(void* ctx) {
  std::cout << "in refcounted_deleter" << std::endl;
  RefcountedDeleterContext& ctx_ = *reinterpret_cast<RefcountedDeleterContext*>(ctx);
  ctx_.refcount--;
  std::cout << "  refcount after decref: " << ctx_.refcount << std::endl;
  if (ctx_.refcount == 0) {
    ctx_.concrete_deleter(ctx_.ptr);

    delete &ctx_;
    std::cout << "  context has been deleted" << std::endl;
  }
}

// This is a shortened version of `UniqueVoidPtr` taken from `c10/util/UniqueVoidPtr.h`
class UniqueVoidPtr {
 private:
  void* data_;
  std::unique_ptr<void, DeleterFnPtr> ctx_;
 public:
  UniqueVoidPtr(void* data, void* ctx, DeleterFnPtr ctx_deleter)
    : data_(data), ctx_(ctx, ctx_deleter) {}

  void clear() {
    std::cout << "in UniqueVoidPtr::clear" << std::endl;
    ctx_ = nullptr;
    data_ = nullptr;
  }
};

int main() {
  void* buffer1 = (void*)1;

  // This is currently how `UniqueVoidPtr`s within `DataPtr` are created
  UniqueVoidPtr ptr1(buffer1, buffer1, &example_cpu_deleter);
  ptr1.clear();

  std::cout << "----------------------" << std::endl;

  void* buffer2 = (void*)2;
  // I think the context has to be created on the heap like this so that
  // `refcounted_deleter` can delete the context when refcount goes to 0
  RefcountedDeleterContext* refcount_ctx = new RefcountedDeleterContext(buffer2, &example_cpu_deleter);
  UniqueVoidPtr ptr2(refcount_ctx->ptr, (void*)refcount_ctx, &refcounted_deleter);
  
  // Create a second UniqueVoidptr that points to the same data
  refcount_ctx->incref();
  UniqueVoidPtr ptr3(refcount_ctx->ptr, (void*)refcount_ctx, &refcounted_deleter);

  ptr3.clear();
  ptr2.clear();

  std::cout << "----------------------" << std::endl;
}
