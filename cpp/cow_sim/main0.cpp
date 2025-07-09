#include <memory>
#include <iostream>

static int next_data_ptr_value_ = 1;

class DataPtr {
 public:
  DataPtr() : value_(next_data_ptr_value_++) {}

  int value() const {
    return value_;
  }

 private:
  int value_;
};

class StorageImpl {
 public:
  int data_ptr() const {
    return data_ptr_.value();
  }

 private:
  DataPtr data_ptr_;
};

class Storage {
 public:
  Storage() : storage_impl_(std::make_shared<StorageImpl>()) {}

  int data_ptr() const {
    return storage_impl_->data_ptr();
  }

 private:
  std::shared_ptr<StorageImpl> storage_impl_;
};


class TensorImpl {
 public:
  Storage& storage() {
    return storage_;
  }

  int data_ptr() const {
    return storage_.data_ptr();
  }

 private:
  Storage storage_;
};

int main() {
  TensorImpl a;
  TensorImpl b;
  std::cout << a.data_ptr() << std::endl;
  std::cout << b.data_ptr() << std::endl;
}