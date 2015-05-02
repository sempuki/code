#ifndef SRC_CORE_COMMON_HPP_
#define SRC_CORE_COMMON_HPP_

using namespace std::literals;  // NOLINT

namespace core {

inline void tolower(std::string &str) {  // NOLINT
  for (auto &ch : str) {
    ch = std::tolower(ch);
  }
}

namespace bits {

constexpr bool ispow2(uint64_t value) {
  return (value & (value - 1)) == 0;
}

constexpr size_t log2(uint64_t value) {
  return value == 1? 0 : log2(value >> 1) + 1;
}

}  // namespace bits

}  // namespace core


#endif  // SRC_CORE_COMMON_HPP_
