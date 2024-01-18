#include <string>
#include <iostream>

int main() {
  std::string a = std::string("hello there");
  std::string b = std::string(a, ", you");

  std::cout << b << std::endl;
}
