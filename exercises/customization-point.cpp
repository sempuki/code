#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>

// Needs clang-10

namespace std::impl::customize_on {
void customize_on() = delete;  // poison pill overloads in this namespace

struct customization_point {
  template <typename CustomizationPoint, typename... Args>
  requires requires(CustomizationPoint cp, Args&&... args) {
    customize_on((CustomizationPoint &&) cp, (Args &&) args...);
  }
  constexpr auto operator()(CustomizationPoint cp, Args&&... args) const
      -> decltype(customize_on((CustomizationPoint &&) cp, (Args &&) args...)) {
    return customize_on((CustomizationPoint &&) cp, (Args &&) args...);
  }
};
}  // namespace std::impl::customize_on

namespace std {
inline constexpr impl::customize_on::customization_point customize_on = {};

template <auto& CustomizationPoint>
using customization_point = decay_t<decltype(CustomizationPoint)>;
}  // namespace std

// =============================================================================

namespace std::execution {
inline constexpr struct execute_customization_point {
  template <typename Executor, typename... Args>
  auto operator()(Executor&& ex, Args&&... args) const
      -> std::invoke_result_t<decltype(std::customize_on),
                              execute_customization_point, Executor, Args...> {
    return std::customize_on(*this, std::forward<Executor>(ex),
                             std::forward<Args>(args)...);
  }
} execute;
}  // namespace std::execution

namespace my {
struct Executor {
  template <typename... Args>
  friend void customize_on(std::customization_point<std::execution::execute>,
                           Executor& ex, Args... args) {
    std::cout << "Hello World\n";
    std::invoke(std::forward<Args>(args)...);
  }
};
}  // namespace my

int main() {
  my::Executor executor;
  std::execution::execute(executor, [] { std::cout << "it's me!\n"; });
}
