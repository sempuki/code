#include <memory>
#include <type_traits>

namespace impl {

template <typename>
struct function_interface;

template <typename R, typename... P>
struct function_interface<R(P...)> {
  virtual ~function_interface() = default;
  virtual function_interface<R(P...)>* clone() const = 0;
  virtual R invoke(P...) = 0;
};

template <typename, typename>
struct function_implementation;

template <typename R, typename... P, typename Callable>
struct function_implementation<R(P...), Callable> final
    : public function_interface<R(P...)> {
  function_implementation(Callable c) : callable{std::move(c)} {}
  function_interface<R(P...)>* clone() const override {
    return new function_implementation<R(P...), Callable>{callable};
  }
  R invoke(P... params) override {
    return callable(std::forward<P>(params)...);
  }
  Callable callable;
};

}  // namespace impl

template <typename>
class function;

template <typename R, typename... P>
class function<R(P...)> final {
 public:
  function() = default;

  template <typename F, typename = std::enable_if_t<
                            !std::is_same<std::decay_t<F>, function>::value>>
  function(F&& f)
      : target_{new impl::function_implementation<R(P...), std::decay_t<F>>(
            std::forward<F>(f))} {}

  function(const function& that)
      : target_{that.target_ ? that.target_->clone() : nullptr} {}

  function& operator=(const function& that) {
    if (this != &that) {
      target_.reset(that.target_ ? that.target_->clone() : nullptr);
    }
    return *this;
  }

  R operator()(P... params) {
    return target_->invoke(std::forward<P>(params)...);
  }

 private:
  std::unique_ptr<impl::function_interface<R(P...)>> target_;
};

