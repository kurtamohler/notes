#include <iostream>
#include <functional>

using DeleterFn = std::function<void(void*)>;

// TODO: I think I actually need to do this:
// using DeleterFnPtr = std::shared_ptr<DeleterFn>
using DeleterFnPtr = DeleterFn*;

void cpu_deleter(void* ptr) {
  std::cout << "in cpu_deleter" << std::endl;
}
DeleterFn cpu_deleter_fn = &cpu_deleter;
DeleterFnPtr cpu_deleter_fn_ptr = &cpu_deleter_fn;

class RefcountedDeleter {
 public:
  RefcountedDeleter(DeleterFnPtr unique_deleter)
    : unique_deleter_ptr(unique_deleter),
      refcount_(1),
      shared_deleter_fn([&](void* ptr) {
        return decref(ptr);
      })
  {}

  void decref(void* ptr) {
    std::cout << "in RefcountedDeleter::decref" << std::endl;
    refcount_--;
    std::cout << "  refcount after decref: " << refcount_ << std::endl;
    if (refcount_ <= 0) {
      std::cout << "  calling unique deleter" << std::endl;
      (*unique_deleter_ptr)(ptr);
    }
  } 

  DeleterFnPtr ptr() {
    return &shared_deleter_fn;
  }

  DeleterFnPtr incref() {
    std::cout << "in RefcountedDeleter::incref" << std::endl;
    refcount_++;
    std::cout << "  refcount after incref: " << refcount_ << std::endl;
    return ptr();
  } 

 private:
  DeleterFnPtr unique_deleter_ptr;
  size_t refcount_;
  DeleterFn shared_deleter_fn;
};

int main() {
  DeleterFnPtr a = cpu_deleter_fn_ptr;
  (*a)(nullptr);
  std::cout << "------------------------" << std::endl;

  RefcountedDeleter refcounted_deleter(cpu_deleter_fn_ptr);

  DeleterFnPtr b0 = refcounted_deleter.ptr();
  DeleterFnPtr b1 = refcounted_deleter.incref();

  (*b0)(nullptr);
  (*b1)(nullptr);

}
