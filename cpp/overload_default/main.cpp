#include <iostream>

struct BaseClass {
  void func() const;

  virtual void other_func() const;

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

void BaseClass::other_func() const {
  std::cout << "in BaseClass::other_func" << std::endl;
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
  void other_func() const override {
    std::cout << "in ChildClass1::other_func" << std::endl;
  }

private:
  void func_impl() const final {
    std::cout << "in ChildClass1::func_impl" << std::endl;
    default_func_impl();
  }
};

int main() {
  ChildClass0 a;
  BaseClass* a_ptr = reinterpret_cast<BaseClass*>(&a);
  std::cout << "-------------" << std::endl;
  a.func();
  a.other_func();

  std::cout << "-------------" << std::endl;
  a_ptr->func();
  a_ptr->other_func();

  ChildClass1 b;
  BaseClass* b_ptr = reinterpret_cast<BaseClass*>(&b);

  std::cout << "-------------" << std::endl;
  b.func();
  b.other_func();

  std::cout << "-------------" << std::endl;
  b_ptr->func();
  b_ptr->other_func();
}
