#ifndef SRC_CORE_ERROR_HPP_
#define SRC_CORE_ERROR_HPP_

#include <cerrno>

namespace error {

static constexpr bool nofail = false;

inline bool erred(const std::error_code &code) {
  return static_cast<bool> (code);
}

inline bool erred(bool success) {
  return !success;
}

inline bool erred(void const *pointer) {
  return pointer == nullptr;
}

inline bool erred(size_t count) {
  return count == 0;
}

inline bool erred(int value) {
  return value != 0;
}

inline bool trap(int value, std::error_code &code) {  // NOLINT
  auto has_error = erred(value);
  if (has_error) {
    code.assign(value, std::system_category());
  }
  return has_error;
}

inline bool trap(const std::error_code &error, std::error_code &code) {  // NOLINT
  auto has_error = erred(error);
  if (has_error) {
    code.assign(error.value(), error.category());
  }
  return has_error;
}

template <typename ReturnType>
bool trap(ReturnType &&value, const std::error_condition &cond, std::error_code &code) {  // NOLINT
  auto has_error = erred(value);
  if (has_error) {
    code.assign(cond.value(), cond.category());;
  }
  return has_error;
}

namespace cerrno {

template <typename ReturnType>
bool trap(ReturnType &&value, std::error_code &code) {  // NOLINT
  auto has_error = erred(value);
  if (has_error) {
    code.assign(errno, std::system_category());
  }
  return has_error;
}

}  // namespace cerrno

}  // namespace error

#endif  // SRC_CORE_ERROR_HPP_
