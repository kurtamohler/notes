#include <map>
#include <string>
#include <cstring>
#include <iostream>

struct CharPtrComparator {
  bool operator()(const char* a, const std::string& b) const {
    std::cout << "here?" << std::endl;
    return std::strcmp(a, b.c_str()) < 0;
  }

  bool operator()(const std::string& a, const char* b) const {
    std::cout << "here?" << std::endl;
    return std::strcmp(a.c_str(), b) < 0;
  }

  bool operator()(const std::string& a, const std::string& b) const {
    std::cout << "here" << std::endl;
    return std::strcmp(a.c_str(), b.c_str()) < 0;
  }
};

int main() {
  std::map<std::string, int64_t, CharPtrComparator> my_map;

  my_map["something"] = 100;
  my_map["another thing"] = 394;
  my_map["zebra"] = 3234;
  my_map["cat"] = 2092;

  const char* key = "cat";

  std::map<std::string, int64_t, CharPtrComparator>::iterator it = my_map.find(key);

  if (it != my_map.end()) {
    std::cout << it->second << std::endl;
  }
}
