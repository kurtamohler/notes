#include <string>
#include <iostream>

class MyClass {
 public:
  MyClass(std::string name, float num)
    : name_(name),
      num_(num) {}

  std::string name() {
    return name_;
  }

  float num() {
    return num_;
  }

 private:
  std::string name_;
  float num_;
};

int main() {
  size_t objects_size = 2;
  MyClass* objects = reinterpret_cast<MyClass*>(new unsigned char[objects_size * sizeof(MyClass)]);

  new(&objects[0]) MyClass("name0", 0.123);
  new(&objects[1]) MyClass("name1", 456.789);

  for (size_t i = 0; i < objects_size; i++) {
    std::cout << objects[i].name() << ": " << objects[i].num() << std::endl;
  }

  for (size_t i = 0; i < objects_size; i++) {
    objects[i].~MyClass();
  }

  delete [] reinterpret_cast<unsigned char*>(objects);
}
