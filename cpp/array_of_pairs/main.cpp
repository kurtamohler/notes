#include <string>
#include <array>
#include <stdexcept>
#include <iostream>

class StorageImpl {
 public:
  StorageImpl() {}

  StorageImpl(std::string name) 
    : name_(name)
  {}

 private:
  std::string name_;
};

struct MyPair {
  MyPair() {}

  MyPair(size_t first, StorageImpl second)
    : first(first),
      second(second)
  {}

  size_t first;
  StorageImpl second;
};

class Manager {
 public:
  Manager(size_t capacity) 
    : size_(0),
      capacity_(capacity)
  {
    list_ = new MyPair[capacity_];
  }

  void emplace_back(size_t first, StorageImpl second) {
    if (size_ == capacity_) {
      throw std::runtime_error(
        "Cannot emplace_back, because capacity has been reached");
    }

    list_[size_].first = first;
    list_[size_].second = second;
    size_++;
  }

  ~Manager() {
    delete [] list_;
  }

 private:
  MyPair* list_;
  size_t size_;
  size_t capacity_;
};

int main() {
  Manager m(2);

  m.emplace_back(10, StorageImpl("ten"));
  m.emplace_back(20, StorageImpl("twenty"));
}
