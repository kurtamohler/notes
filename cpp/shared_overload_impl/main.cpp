#include <iostream>

struct BaseClass {
  void func() const;

  virtual void func_impl() const = 0;

protected:
  void default_func_impl() const;
};

void BaseClass::default_func_impl() const {
  std::cout << "in BaseClass::default_func_impl" << std::endl;
}

void BaseClass::func() const {
  std::cout << "in BaseClass::func" << std::endl;
  func_impl();
}


struct ChildClass0 : public BaseClass {
  void func_impl() const final;
};

void ChildClass0::func_impl() const {
  std::cout << "in ChildClass0::func_impl" << std::endl;
  default_func_impl();
}


struct ChildClass1 : public BaseClass {
  void func_impl() const final {
    std::cout << "in ChildClass1::func_impl" << std::endl;
    //default_func_impl();
    std::cout << "do stuff" << std::endl;
  }
};

struct ChildClassWrapper : public BaseClass {
  ChildClassWrapper(BaseClass* other) :
    _other(other) {}

  void func_impl() const final {
    std::cout << "in ChildClassWrapper::func_impl" << std::endl;
    _other->func_impl();
  }

private:
  BaseClass* _other;
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

  std::cout << "-------------" << std::endl;
  ChildClassWrapper a_wrap(&a);
  a.func();

  std::cout << "-------------" << std::endl;
  ChildClassWrapper b_wrap(&b);
  b.func();
}
