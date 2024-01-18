#include <map>
#include <string>
#include <iostream>

int main() {
  std::map<std::string, int> map;

  std::string key0 = "one";

  map[key0] = 1;

  std::cout << "map['one'] = " << map["one"] << std::endl;
  std::cout << "map['one'] = " << map["two"] << std::endl;
}
