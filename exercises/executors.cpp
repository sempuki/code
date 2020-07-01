#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <variant>

// Needs clang-10

// =============================================================================
// See: wg21.link/p1895

namespace std::impl::customization {
void customize() = delete;

struct customizable {
  template <typename Customizable, typename... Args>
  requires requires(Customizable cp, Args &&... args) {
    customize(std::forward<Customizable>(cp), std::forward<Args>(args)...);
  }

  constexpr decltype(auto) operator()(Customizable cp, Args &&... args) const {
    return customize(std::forward<Customizable>(cp), std::forward<Args>(args)...);
  }
};
} // namespace std::impl::customization

namespace std {
inline constexpr impl::customization::customizable customize{};
template <auto &Customizable> using on = std::decay_t<decltype(Customizable)>;

#define CREATE_FORWARDING_CUSTOMIZATION_POINT(namespace_name, customization_name)                  \
  inline constexpr struct namespace_name##_##customization_name {                                  \
    template <typename... Args> decltype(auto) operator()(Args &&... args) const {                 \
      return std::customize(*this, std::forward<Args>(args)...);                                   \
    }                                                                                              \
  } customization_name;
} // namespace std

// =============================================================================
// See: wg21.link/p0443r13

namespace std::execution {
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, set_value);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, set_error);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, set_done);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, execute);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, connect);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, start);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, submit);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, schedule);
CREATE_FORWARDING_CUSTOMIZATION_POINT(execution, bulk_execute);

template <typename Sender, typename Receiver>
using connect_result_t = std::invoke_result_t<decltype(connect), Sender, Receiver>;

template <typename Function, typename Sender> struct as_receiver {
  Function f;
  void set_value() { std::invoke(f); }
  void set_error(std::exception_ptr) { std::terminate(); }
  void set_done() {}
};

template <typename Receiver, typename Sender> struct as_invocable {
  Receiver *r_;
  explicit as_invocable(Receiver &r) : r_(std::addressof(r)) {}
  as_invocable(as_invocable &&other) : r_(std::exchange(other.r_, nullptr)) {}
  ~as_invocable() {
    if (r_) {
      execution::set_done(std::move(*r_));
    }
  }
  void operator()() & try {
    std::execution::set_value(std::move(*r_));
    r_ = nullptr;
  } catch (...) {
    std::execution::set_error(std::move(*r_), std::current_exception());
    r_ = nullptr;
  }
};

template <typename Type, typename Error = std::exception_ptr>
concept receiver = // clang-format off
  std::move_constructible<std::remove_cvref_t<Type>> &&
  std::constructible_from<std::remove_cvref_t<Type>, Type> &&
  requires(remove_cvref_t<Type> &&value, Error &&error) {
    { std::execution::set_done(std::move(value)) } noexcept;
    { std::execution::set_error(std::move(value), std::forward<Error>(error)) } noexcept;
}; // clang-format on

template <typename Type, typename... Args>
concept receiver_of = // clang-format off
  std::execution::receiver<Type> &&
  requires(std::remove_cvref_t<Type> &&value, Args &&... an) {
    std::execution::set_value(std::move(value), std::forward<Args>(an)...);
}; // clang-format on

struct void_receiver { // exposition only
  void set_value() noexcept;
  void set_error(std::exception_ptr) noexcept;
  void set_done() noexcept;
};

template <typename Operation>
concept operation_state = // clang-format off
  std::destructible<Operation> &&
  std::is_object_v<Operation> &&
  requires(Operation &op) {
    { std::execution::start(op) } noexcept;
}; // clang-format on

// template <typename Sender>
// concept sender = // clang-format off
//   std::move_constructible<std::remove_cvref_t<Sender>> &&
//   !requires {
//     typename sender_traits<std::remove_cvref_t<Sender>>::__unspecialized; // exposition only
// }; // clang-format on

// template <typename Sender, typename Receiver>
// concept sender_to = // clang-format off
//   std::execution::sender<Sender> &&
//   std::execution::receiver<Receiver> &&
//   requires (Sender&& s, Receiver&& r) {
//     std::execution::connect((Sender&&) s, (Receiver&&) r);
// }; // clang-format on

template <typename Sender> concept sender = true; // TODO

template <typename Sender, typename Receiver>
concept sender_to = // clang-format off
  std::execution::sender<Sender> &&
  std::execution::receiver<Receiver> && 
  requires(Sender&& s, Receiver&& r) {
    std::execution::connect(std::forward<Sender>(s), std::forward<Receiver>(r));
}; // clang-format of

template <template <template <class...> class Tuple, template <class...> class Variant> class>
struct has_value_types; // exposition only

template <template <template <class...> class Variant> class>
struct has_error_types; // exposition only

template <class Sender>
concept has_sender_types = // exposition only
    requires {
  typename has_value_types<Sender::template value_types>;
  typename has_error_types<Sender::template error_types>;
  typename std::bool_constant<Sender::sends_done>;
};

// == if has_sender_types<Sender> is true
// template <typename Sender> struct sender_traits_base {
//   template <template <class...> class Tuple, template <class...> class Variant>
//   using value_types = typename Sender::template value_types<Tuple, Variant>;
//
//   template <template <class...> class Variant>
//   using error_types = typename Sender::template error_types<Variant>;
//
//   static constexpr bool sends_done = Sender::sends_done;
// };

// == if executor_of_impl<Sender, as_invocable<void_receiver, Sender> is true
// template <typename Sender> struct sender_traits_base {
//   template <template <class...> class Tuple, template <class...> class Variant>
//   using value_types = Variant<Tuple<>>;
//
//   template <template <class...> class Variant> using error_types = Variant<std::exception_ptr>;
//
//   static constexpr bool sends_done = true;
// };

// == if std::derived_from<Sender, as_invocable<void_receiver, Sender>> is true
template <typename Sender> struct sender_traits_base {};

template <typename Sender> struct sender_traits : public sender_traits_base<Sender>{};

template<typename Sender>
concept typed_sender = // clang-format off
  std::execution::sender<Sender> &&
  std::execution::has_sender_types<std::remove_cvref_t<Sender>>
; // clang-format on

// template <typename Sender>
// concept scheduler = // clang-format off
//   std::copy_constructible<std::remove_cvref_t<Sender>> &&
//   std::equality_comparable<std::remove_cvref_t<Sender>> &&
//   requires(E&& e) {
//     std::execution::schedule(std::forward<E>(e));
// }; // clang-format on

template <typename Sender> concept scheduler = true; // TODO

template <typename Executor, typename Function>
concept executor_of_impl = // clang-format off
  std::invocable<std::remove_cvref_t<Function> &> &&
  std::constructible_from<std::remove_cvref_t<Function>, Function> &&
  std::move_constructible<std::remove_cvref_t<Function>> &&
  std::copy_constructible<Executor> &&
  std::is_nothrow_copy_constructible_v<Executor> &&
  std::equality_comparable<Executor> &&
  requires(const Executor &ex, Function &&fn) {
    std::execution::execute(ex, std::forward<Function>(fn));
}; // clang-format on

struct invocable_archetype {
  void operator()() & {}
};

static_assert(std::is_invocable_v<std::execution::invocable_archetype &>);

template <typename Executor>
concept executor = // clang-format off
  std::execution::executor_of_impl<Executor, std::execution::invocable_archetype>
; // clang-format on

template <typename Executor, typename Function>
concept executor_of = // clang-format off
  std::execution::executor<Executor> &&
  std::execution::executor_of_impl<Executor, Function>
; // clang-format on

struct context_t {};
struct blocking_t {};
struct blocking_adaptation_t {};
struct relationship_t {};
struct outstanding_work_t {};
struct bulk_guarantee_t {};
struct mapping_t {};
template <typename ProtoAllocator> struct allocator_t;

constexpr context_t context;
constexpr blocking_t blocking;
constexpr blocking_adaptation_t blocking_adaptation;
constexpr relationship_t relationship;
constexpr outstanding_work_t outstanding_work;
constexpr bulk_guarantee_t bulk_guarantee;
constexpr mapping_t mapping;
// constexpr allocator_t<void> allocator;

} // namespace std::execution

// =============================================================================

namespace my {

enum class Error { A, B, C };

template <typename Sender, typename Receiver>
concept sender_to = // clang-format off
  std::execution::sender<Sender> &&
  std::execution::receiver<Receiver> && 
  requires(Sender&& s, Receiver&& r) {
    std::execution::connect(std::forward<Sender>(s), std::forward<Receiver>(r));
}; // clang-format of

class Sender {
public:
  // using value_types = std::variant<std::tuple<>>;
  // using error_types = std::variant<>;
  static constexpr bool sends_done = true;

private:
  friend void customize(std::on<std::execution::execute>) {}
};

template <typename Type> class Receiver {
public:
private:
  friend void customize(std::on<std::execution::set_done>, Receiver &self) {
    std::cout << "Set Done: \n";
  }
  friend void customize(std::on<std::execution::set_error>, Receiver &self,
                           Error error) {
    std::cout << "Set Error: " << static_cast<int>(error) << "\n";
  }
  friend void customize(std::on<std::execution::set_value>, Receiver &self,
                           Type value) {
    std::cout << "Set Value: " << value << "\n";
  }
};

class Executor {
private:
  template <typename... Args>
  friend void customize(std::on<std::execution::execute>, Executor &self,
                           Args &&... args) {
    std::cout << "Executing: ";
    std::invoke(std::forward<Args>(args)...);
  }
};
} // namespace my

// =============================================================================

int main() {
  my::Executor executor;
  std::execution::execute(executor, [] { std::cout << "it's me!\n"; });
}
