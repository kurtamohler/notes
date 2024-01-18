#include <iostream>

class InnerThing {
 public:
  void func_const() const {
    std::cout << "in InnerThing::func_const()" << std::endl;
  }

  void func_nonconst() {
    std::cout << "in InnerThing::func_nonconst()" << std::endl;
  }
};

class Thing {
 public:
  const InnerThing& get() const {
    return inner_thing_;
  }

  InnerThing& mutable_get() {
    return inner_thing_;
  }

private:
  InnerThing inner_thing_;
};

int main() {
  Thing thing;
  thing.get().func_const();
  thing.mutable_get().func_const();
  thing.mutable_get().func_nonconst();
}
