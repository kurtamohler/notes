#include <variant>
#include <iostream>

int main() {
  std::variant<void*, const void*> a = static_cast<void*>(nullptr);

  struct IsNullptrVisitor {
    bool operator()(void* data) const {
      return data == nullptr;
    }
    bool operator()(const void* data) const {
      return data == nullptr;
    }
  };


  std::visit(IsNullptrVisitor(), a);
}
