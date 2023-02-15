#include <atomic>
#include <vector>
#include <utility>
#include <iostream>

namespace c10 {

class PyInterpreter {
};

class PyObject {
};

class PyObjectSlot;
inline void swap(PyObjectSlot& lhs, PyObjectSlot& rhs);

class PyObjectSlot {
 public:
  PyObjectSlot()
    : pyobj_interpreter_(nullptr) {}

  PyObjectSlot(PyObjectSlot&& other)
    : pyobj_interpreter_(other.pyobj_interpreter_.load()) {}

  PyObjectSlot& operator=(const PyObjectSlot& other) {
    pyobj_interpreter_ = other.pyobj_interpreter_.load();
    pyobj_ = other.pyobj_;
    return *this;
  }

  friend void swap(PyObjectSlot& lhs, PyObjectSlot& rhs);

  void set_pyobj_interpreter(PyInterpreter* pyobj_interpreter) {
    pyobj_interpreter_ = pyobj_interpreter;
  }

  PyInterpreter* pyobj_interpreter() {
    return pyobj_interpreter_;
  }

 private:
  std::atomic<PyInterpreter*> pyobj_interpreter_;
  PyObject* pyobj_;
};

inline void swap(PyObjectSlot& lhs, PyObjectSlot& rhs) {
  std::swap(lhs.pyobj_, rhs.pyobj_);
  lhs.pyobj_interpreter_.exchange(
    rhs.pyobj_interpreter_.exchange(lhs.pyobj_interpreter_));
}


class StorageImpl {
 private:
  PyObjectSlot pyobj_slot_;
};

} // namespace c10


int main() {
  // Try moving PyObjectSlot

  c10::PyObjectSlot a;

  c10::PyInterpreter interp_a;
  a.set_pyobj_interpreter(&interp_a);

  c10::PyObjectSlot b(std::move(a));

  std::vector<std::pair<std::size_t, c10::PyObjectSlot>> v;
  v.emplace_back(0, std::move(b));

  std::cout << "a.pyobj_interpreter():           " << a.pyobj_interpreter() << std::endl;
  std::cout << "b.pyobj_interpreter():           " << b.pyobj_interpreter() << std::endl;
  std::cout << "v[0].second.pyobj_interpreter(): " << v[0].second.pyobj_interpreter() << std::endl;
  std::cout << std::endl;


  // Try swapping PyObjectSlots

  c10::PyObjectSlot x;
  c10::PyObjectSlot y;

  c10::PyInterpreter interp_x;
  c10::PyInterpreter interp_y;

  x.set_pyobj_interpreter(&interp_x);
  y.set_pyobj_interpreter(&interp_y);

  std::cout << "x.pyobj_interpreter(): " << x.pyobj_interpreter() << std::endl;
  std::cout << "y.pyobj_interpreter(): " << y.pyobj_interpreter() << std::endl;

  std::cout << "swap" << std::endl;
  std::swap(x, y);

  std::cout << "x.pyobj_interpreter(): " << x.pyobj_interpreter() << std::endl;
  std::cout << "y.pyobj_interpreter(): " << y.pyobj_interpreter() << std::endl;
}
