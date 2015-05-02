#ifndef SRC_CORE_RANDOM_HPP_
#define SRC_CORE_RANDOM_HPP_

#include <random>

namespace core {

static std::random_device static_random_device;

template <typename Type, template <typename> class Distribution, typename Engine>
struct random_generator_base {
  using result_type = Type;
  using distribution_type = Distribution<Type>;
  using engine_type = Engine;

  Engine engine {static_random_device()};
  Distribution<Type> distribution;

  template <typename ...Args>
  random_generator_base(Args &&...params) :  // NOLINT
    distribution {typename Distribution<Type>::param_type(params...)} {}

  template <typename ...Args>
  void configure(Args &&...params) {
    distribution.param(typename Distribution<Type>::param_type(params...));
  }

  Type operator()() { return distribution(engine); }
};

template <typename Type, typename Engine, bool integral, bool floatingpoint>
struct uniform_random_generator_impl;

template <typename Type, typename Engine>
struct uniform_random_generator_impl<Type, Engine, true, false> :
  public random_generator_base<Type, std::uniform_int_distribution, Engine> {
  using random_generator_base<Type, std::uniform_int_distribution, Engine>::random_generator_base;
};

template <typename Type, typename Engine>
struct uniform_random_generator_impl<Type, Engine, false, true> :
  public random_generator_base<Type, std::uniform_real_distribution, Engine> {
  using random_generator_base<Type, std::uniform_real_distribution, Engine>::random_generator_base;
};

template <typename Type, typename Engine>
using uniform_random_generator_specialization = uniform_random_generator_impl<Type, Engine,
      std::is_integral<Type>::value, std::is_floating_point<Type>::value>;

template <typename Generator>
struct generator_iterator :
  public std::iterator<std::input_iterator_tag, typename Generator::result_type> {
    Generator &generator;
    explicit generator_iterator(Generator &generator) :  // NOLINT
      generator {generator} {}

    generator_iterator &operator++() { return *this; }
    generator_iterator &operator++(int post) { return *this; }
    typename Generator::result_type operator*() { return generator(); }
};

template <typename Type, typename Engine = std::default_random_engine>
struct uniform_random_generator : public uniform_random_generator_specialization<Type, Engine> {
  struct iterator : generator_iterator<uniform_random_generator> {
    iterator() : generator_iterator<uniform_random_generator> {generator} {};
    uniform_random_generator generator;
  };

  using uniform_random_generator_specialization<Type, Engine>::uniform_random_generator_specialization;
};

template <typename Type, template <typename> class Distribution, typename Engine = std::default_random_engine>
struct random_generator : public random_generator_base<Type, Distribution, Engine> {
  struct iterator : generator_iterator<random_generator> {
    iterator() : generator_iterator<random_generator> {generator} {};
    random_generator generator;
  };

  using random_generator_base<Type, Distribution, Engine>::random_generator_base;
};

}  // namespace core

#endif   // SRC_CORE_RANDOM_HPP_
