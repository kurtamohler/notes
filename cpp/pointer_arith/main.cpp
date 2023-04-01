#include <iostream>

int main() {
  long* a = reinterpret_cast<long*>(100);

  std::cout << a << std::endl;
  std::cout << a+1 << std::endl;
}
