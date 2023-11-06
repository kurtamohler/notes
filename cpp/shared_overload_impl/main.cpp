#include <iostream>

struct BaseClass {
  void func() const;

protected:
  void default_func_impl() const;

private:
  virtual void func_impl() const = 0;
};

void BaseClass::default_func_impl() const {
  std::cout << "in BaseClass::default_func_impl" << std::endl;
}

void BaseClass::func() const {
  std::cout << "in BaseClass::func" << std::endl;
  func_impl();
}


struct ChildClass0 : public BaseClass {
private:
  void func_impl() const final;
};

void ChildClass0::func_impl() const {
  std::cout << "in ChildClass0::func_impl" << std::endl;
  default_func_impl();
}


struct ChildClass1 : public BaseClass {
private:
  void func_impl() const final {
    std::cout << "in ChildClass1::func_impl" << std::endl;
    default_func_impl();
  }
};

int main() {
  ChildClass0 a;

  std::cout << "-------------" << std::endl;
  a.func();

  std::cout << "-------------" << std::endl;
  reinterpret_cast<BaseClass*>(&a)->func();

  ChildClass1 b;

  std::cout << "-------------" << std::endl;
  b.func();

  std::cout << "-------------" << std::endl;
  reinterpret_cast<BaseClass*>(&b)->func();
}
