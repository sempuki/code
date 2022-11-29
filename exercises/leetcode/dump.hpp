#include <iostream>
#include <ranges>
#include <string_view>

template <typename T>
concept OstreamPrintable = requires(T a) {
  {std::declval<std::ostream&>() << a};
};

void dump(OstreamPrintable auto&& e, std::string_view delim = {}) {
  if (delim.size()) {
    std::cout << "-- " << delim << "\n";
  }
  std::cout << e << "\n";
}

void dump(std::ranges::range auto&& r, std::string_view delim = {}) {
  if (delim.size()) {
    std::cout << "-- " << delim << "\n";
  }
  for (auto&& e : r) {
    std::cout << e << ", ";
  }
  std::cout << "\n";
}

void dump(...) {
  std::cout << "Failed to dump.\n";
}
