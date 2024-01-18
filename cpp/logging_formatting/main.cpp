#include "extras.h"

#include <string>
#include <iostream>
#include <functional>

namespace c10 {

class Log_A {
 public:
  Log_A(
      const char* component_alias,
      int64_t py_log_level,
      const SourceLocation& source_location,
      std::string msg)
      : component_alias_(std::string(component_alias)),
        py_log_level_(py_log_level),
        source_location_(source_location),
        msg_(std::move(msg)) {}

  Log_A(
      const char* component_alias,
      int64_t py_log_level,
      const SourceLocation& source_location,
      const char* msg)
      : component_alias_(std::string(component_alias)),
        py_log_level_(py_log_level),
        source_location_(source_location),
        msg_(std::string(msg)) {}

  const std::string& component_alias() const {
    return component_alias_;
  }

  int64_t py_log_level() const {
    return py_log_level_;
  }

  const SourceLocation& source_location() const {
    return source_location_;
  }

  const std::string& msg() const {
    return msg_;
  }

 private:
  std::string component_alias_;
  int64_t py_log_level_;
  SourceLocation source_location_;
  std::string msg_;
};

class Log_B {
 public:
  Log_B(
      const char* component_alias,
      int64_t py_log_level,
      const SourceLocation& source_location,
      std::function<std::string(void)> msg)
      : component_alias_(std::string(component_alias)),
        py_log_level_(py_log_level),
        source_location_(source_location),
        msg_(std::move(msg)) {}

  const std::string& component_alias() const {
    return component_alias_;
  }

  int64_t py_log_level() const {
    return py_log_level_;
  }

  const SourceLocation& source_location() const {
    return source_location_;
  }

  const std::string msg() const {
    return msg_();
  }

 private:
  std::string component_alias_;
  int64_t py_log_level_;
  SourceLocation source_location_;
  std::function<std::string(void)> msg_;
};

// Issue a log with a given message
void log_A(const Log_A& log) {
  std::cout << "LOG_A(level: " << log.py_log_level()
    << ", component: " << log.component_alias()
    << "): " << log.msg() << std::endl;
}

void log_B(const Log_B& log) {
  std::cout << "LOG_B(level: " << log.py_log_level()
    << ", component: " << log.component_alias()
    << "): " << log.msg() << std::endl;
}

} // namespace c10

#define TORCH_LOG_A(component_alias, log_level, ...)           \
  ::c10::log_A(::c10::Log_A(                                     \
      component_alias,                                       \
      log_level,                                             \
      {__func__, __FILE__, static_cast<uint32_t>(__LINE__)}, \
      WARNING_MESSAGE_STRING(__VA_ARGS__)));

#define TORCH_LOG_B(component_alias, log_level, ...)           \
  ::c10::log_B(::c10::Log_B(                                     \
      component_alias,                                       \
      log_level,                                             \
      {__func__, __FILE__, static_cast<uint32_t>(__LINE__)}, \
      [=]{ return WARNING_MESSAGE_STRING(__VA_ARGS__); }));

int main() {
  int some_number = 12345;
  std::string some_string = "some string";
  TORCH_LOG_A("some component", 12,
    "some log", " message with a number (", some_number, ") and a string (\"", some_string, "\")");
  TORCH_LOG_A("some other component", 24,
    "another log", " message with a number (", some_number, ") and a string (\"", some_string, "\")");

  ::c10::log_A(::c10::Log_A(
      "component 2",
      8,
      {__func__, __FILE__, static_cast<uint32_t>(__LINE__)},
      WARNING_MESSAGE_STRING("my ", "message")));

      
  c10::Log_B log0 = ::c10::Log_B(
      "component 2",
      8,
      {__func__, __FILE__, static_cast<uint32_t>(__LINE__)},
      [=]{ return WARNING_MESSAGE_STRING("message with a number (", some_number, ")");});

  some_number += 1;
  ::c10::log_B(log0);

  TORCH_LOG_B("some component", 12,
    "some log", " message with a number (", some_number, ") and a string (\"", some_string, "\")");
}
