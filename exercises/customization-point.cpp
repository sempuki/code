#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <type_traits>

// Use >= clang-10

// =============================================================================
// Customization point framework.

namespace std::impl::customization {
// Poison pill overloads in this namespace.
void customize_on() = delete;

// Customization point internal type.
struct customization_point {
  // In-place concept definition.
  template <typename Customizable, typename... Args>
  requires requires(Customizable cp, Args &&... args) {
    customize_on(std::forward<Customizable>(cp), std::forward<Args>(args)...);
  }

  // Customization invocation.
  constexpr decltype(auto) operator()(Customizable cp, Args &&... args) const {
    // Unqualified call via ADL.
    return customize_on(std::forward<Customizable>(cp), std::forward<Args>(args)...);
  }
};
} // namespace std::impl::customization

namespace std {
// Expose meta customization point object as `std::customize_on`.
inline constexpr impl::customization::customization_point customize_on{};

// Expose customization dispatch type as `std::customization_point`.
template <auto &Customizable> using customization_point = decay_t<decltype(Customizable)>;
} // namespace std

// =============================================================================
// Customization point defintion.

namespace std::execution {
// Customize some aspect of execution.
inline constexpr struct execute_customization_point {
  template <typename Executor, typename... Args>
  decltype(auto) operator()(Executor &&ex, Args &&... args) const {
    return std::customize_on(*this, std::forward<Executor>(ex), std::forward<Args>(args)...);
  }
} execute; // Expose the customization point object.
} // namespace std::execution

// =============================================================================
// Customize `my::Executor` behavior.

namespace my {
class Executor {
private:
  // Hidden friends are available for ADL.
  template <typename... Args>
  friend void customize_on(std::customization_point<std::execution::execute>, Executor &ex,
                           Args... args) {
    std::cout << "Hello World\n";
    std::invoke(std::forward<Args>(args)...);
  }
};
} // namespace my

// =============================================================================
// Show customized behavior.

int main() {
  my::Executor executor;
  std::execution::execute(executor, [] { std::cout << "it's me!\n"; });
}
