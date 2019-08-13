#include <type_traits>

#pragma once

template <typename Type, typename = void>
struct signature_detector;

template <typename F>
struct signature_detector<
    F, typename std::conditional<true, void, decltype(&F::operator())>::type> {
  using signature_type =
      typename signature_detector<decltype(&F::operator())>::signature_type;
};

template <typename Result, typename... Args>
struct signature_detector<Result(Args...)> {
  using signature_type = Result(Args...);
};

template <typename Result, typename... Args>
struct signature_detector<Result (*)(Args...)> {
  using signature_type = Result(Args...);
};

#define DEFINE_SIGNATURE_DETECTOR_FOR_METHODS(QUALIFIERS)            \
  template <typename Result, class Class, typename... Args>          \
  struct signature_detector<Result (Class::*)(Args...) QUALIFIERS> { \
    using signature_type = Result(Args...) QUALIFIERS;               \
  };

DEFINE_SIGNATURE_DETECTOR_FOR_METHODS()
DEFINE_SIGNATURE_DETECTOR_FOR_METHODS(&&)
DEFINE_SIGNATURE_DETECTOR_FOR_METHODS(&)
DEFINE_SIGNATURE_DETECTOR_FOR_METHODS(const)
DEFINE_SIGNATURE_DETECTOR_FOR_METHODS(const &&)
DEFINE_SIGNATURE_DETECTOR_FOR_METHODS(const &)

