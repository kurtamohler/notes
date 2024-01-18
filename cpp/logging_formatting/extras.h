#pragma once

#include <string>
#include <sstream>
#include <ostream>

namespace c10 {

namespace detail {

struct CompileTimeEmptyString {
  operator const std::string&() const {
    static const std::string empty_string_literal;
    return empty_string_literal;
  }
  operator const char*() const {
    return "";
  }
};

template <typename T>
struct CanonicalizeStrTypes {
  using type = const T&;
};

template <size_t N>
struct CanonicalizeStrTypes<char[N]> {
  using type = const char*;
};

inline std::ostream& _str(std::ostream& ss) {
  return ss;
}

template <typename T>
inline std::ostream& _str(std::ostream& ss, const T& t) {
  ss << t;
  return ss;
}

template <>
inline std::ostream& _str<CompileTimeEmptyString>(
    std::ostream& ss,
    const CompileTimeEmptyString&) {
  return ss;
}

template <typename T, typename... Args>
inline std::ostream& _str(std::ostream& ss, const T& t, const Args&... args) {
  return _str(_str(ss, t), args...);
}

template <typename... Args>
struct _str_wrapper final {
  static std::string call(const Args&... args) {
    std::ostringstream ss;
    _str(ss, args...);
    return ss.str();
  }
};

// Specializations for already-a-string types.
template <>
struct _str_wrapper<std::string> final {
  // return by reference to avoid the binary size of a string copy
  static const std::string& call(const std::string& str) {
    return str;
  }
};

template <>
struct _str_wrapper<const char*> final {
  static const char* call(const char* str) {
    return str;
  }
};

// For c10::str() with an empty argument list (which is common in our assert
// macros), we don't want to pay the binary size for constructing and
// destructing a stringstream or even constructing a string.
template <>
struct _str_wrapper<> final {
  static CompileTimeEmptyString call() {
    return CompileTimeEmptyString();
  }
};

} // namespace detail

struct SourceLocation {
  const char* function;
  const char* file;
  uint32_t line;
};

// Convert a list of string-like arguments into a single string.
template <typename... Args>
inline decltype(auto) str(const Args&... args) {
  return detail::_str_wrapper<
      typename detail::CanonicalizeStrTypes<Args>::type...>::call(args...);
}

} // namespace c10

#define WARNING_MESSAGE_STRING(...) ::c10::str(__VA_ARGS__)
