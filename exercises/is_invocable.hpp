#pragma once

#include <type_traits>
#include <utility>

namespace detail {

template <class Type>
struct class_from_pointer {};

template <class Result, class ClassType>
struct class_from_pointer<Result ClassType::*> {
  using type = ClassType;
};

template <typename Type>
using class_from_pointer_t = typename class_from_pointer<Type>::type;

template <class F, class Arg0>
using enable_if_memfun_base =
    std::enable_if_t<std::is_member_function_pointer<std::decay_t<F>>() &&
                     std::is_base_of<class_from_pointer_t<std::decay_t<F>>,
                                     std::decay_t<Arg0>>()>;

template <class F, class Arg0>
using enable_if_memfun_nonbase =
    std::enable_if_t<std::is_member_function_pointer<std::decay_t<F>>() &&
                     !std::is_base_of<class_from_pointer_t<std::decay_t<F>>,
                                      std::decay_t<Arg0>>()>;

struct convertible_from_any_type {
  convertible_from_any_type(...);
};

// not convertable to any type
struct test_fail_type {
  ~test_fail_type() = delete;
};

// Worst match in overload set due to need for implicit conversion
template <class... Args>
auto test_invoke_syntax(convertible_from_any_type, Args &&... args)
    -> test_fail_type;

// The compiler won't recongize tests as overloads without explicit trailing
// decltype
#define DECLTYPE_RETURN__(x) \
  ->decltype(x) { return x; }

// Syntax for invoking member functions via base class
template <class F, class Arg0, class... Args,
          class = enable_if_memfun_base<F, Arg0>>
inline auto test_invoke_syntax(F &&f, Arg0 &&a, Args &&... args)
    DECLTYPE_RETURN__((std::forward<Arg0>(a).*f)(std::forward<Args>(args)...));

// Syntax for invoking member functions not via base class
template <class F, class Arg0, class... Args,
          class = enable_if_memfun_nonbase<F, Arg0>>
inline auto test_invoke_syntax(F &&f, Arg0 &&a, Args &&... args)
    DECLTYPE_RETURN__(((*std::forward<Arg0>(a)).*
                       f)(std::forward<Args>(args)...));

// Syntax for invoking non-member callable objects (lambdas, functions, etc.)
template <class F, class... Args>
inline auto test_invoke_syntax(F &&f, Args &&... args)
    DECLTYPE_RETURN__(std::forward<F>(f)(std::forward<Args>(args)...));

// Invoking via pointer to members is not supported; move to std::is_invocable
// when available

#undef DECLTYPE_RETURN__

}  // namespace detail

template <class Result, class F, class... Args>
struct is_invocable {
 private:
  // The overload is valid if the syntax is valid when tested with F,
  // and is a better match in the overload set than (...)
  template <class T, class... As>
  static auto test(int)
      -> decltype(detail::test_invoke_syntax(std::declval<T>(),
                                             std::declval<As>()...));

  // The base overload that indicates the syntax with invalid with F
  template <class T, class... As>
  static detail::test_fail_type test(...);

  // Run the test with given arguments
  using test_result = decltype(test<F, Args...>(0));

 public:
  static constexpr bool value = std::conditional_t<
      !std::is_same<test_result, detail::test_fail_type>(),
      std::conditional_t<std::is_void<Result>::value, std::true_type,
                         std::is_convertible<test_result, Result>>,
      std::false_type>();
};

template <class F, class Signature>
struct is_invocable_with {};

#define DEFINE_INVOCABLE_WITH_QUALIFIERS(QUALIFIERS)        \
  template <class F, class Result, class... Args>           \
  struct is_invocable_with<F, Result(Args...) QUALIFIERS> { \
    static constexpr bool value =                           \
        is_invocable<Result, F QUALIFIERS, Args...>::value; \
  };

// NOTE: noexecpt is part of the type system in C++17

DEFINE_INVOCABLE_WITH_QUALIFIERS()
DEFINE_INVOCABLE_WITH_QUALIFIERS(&&)
DEFINE_INVOCABLE_WITH_QUALIFIERS(const)
