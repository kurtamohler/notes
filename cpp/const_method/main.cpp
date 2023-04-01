#include <iostream>

class A {
public:
  A(int val) : val_(val) {}

  int get_val() const {
    return val_;
  }

private:
  int val_;
};

class B {
public:
  B(int val) : a_(val) {}

  A* get_a() {
    return &a_;
  }

  const A* get_a() const {
    return &a_;
  }

private:
  A a_;
};


int main() {
  B b0(0);
  A* a0 = b0.get_a();

  std::cout << a0->get_val() << std::endl;

  const B b1(1);
  const A* a1 = b1.get_a();

  std::cout << a1->get_val() << std::endl;
}
