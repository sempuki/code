#include <cassert>
#include <functional>
#include <iostream>
#include <memory>

struct TestNotCallable {};

struct TestDestroy {
  TestDestroy(TestDestroy&&) = default;
  ~TestDestroy() { destroyed++; }
  static size_t destroyed;

  void operator()() { std::cout << "Destroyable\n"; }
};

size_t TestDestroy::destroyed = 0;

struct TestCopy {
  TestCopy() = default;
  ~TestCopy() = default;

  TestCopy(const TestCopy&) = default;
  TestCopy& operator=(const TestCopy&) = default;

  TestCopy(TestCopy&&) = default;
  TestCopy& operator=(TestCopy&&) = default;

  void operator()() { std::cout << "Copyable\n"; }
};

struct TestMoveOnly {
  TestMoveOnly() = default;
  ~TestMoveOnly() = default;

  TestMoveOnly(const TestMoveOnly&) = delete;
  TestMoveOnly& operator=(const TestMoveOnly&) = delete;

  TestMoveOnly(TestMoveOnly&&) = default;
  TestMoveOnly& operator=(TestMoveOnly&&) = default;

  void operator()() { std::cout << "Move Only\n"; }

  std::unique_ptr<int> p;
};

auto test_empty_lambda = [] {};
void test_empty_function() {}

namespace {
template <typename Signature>
struct callable_eraser {};

template <typename Return, typename... Args>
struct callable_eraser<Return(Args...)> {
  template <typename Erased>
  static void op_move_construct(void* src, void* dst) {
    auto* recovered_src = reinterpret_cast<Erased*>(src);
    auto* recovered_dst = reinterpret_cast<Erased*>(dst);

    new (recovered_dst) Erased(std::move(*recovered_src));
  }

  template <typename Erased>
  static void op_move_assign(void* src, void* dst) {
    auto* recovered_src = reinterpret_cast<Erased*>(src);
    auto* recovered_dst = reinterpret_cast<Erased*>(dst);

    recovered_dst->~Erased();
    new (recovered_dst) Erased(std::move(*recovered_src));
  }

  template <typename Erased>
  static void op_destroy(void* target) {
    auto* recovered = reinterpret_cast<Erased*>(target);

    recovered->~Erased();
  }

  template <typename Erased>
  static void op_delete(void* target) {
    auto* recovered = reinterpret_cast<Erased*>(target);

    delete recovered;
  }

  template <typename Erased>
  static Return op_call(void* target, Args&&... args) {
    auto* recovered = reinterpret_cast<Erased*>(target);

    return (*recovered)(std::forward<Args>(args)...);
  }
};

}  // namespace

namespace std {

template <class Sig>
class mofunction {};

template <class R, class... ArgTypes>
class mofunction<R(ArgTypes...)> {
 public:
  using result_type = R;

  mofunction() noexcept;                // no bool
  mofunction(nullptr_t) noexcept;       // no bool
  mofunction(mofunction&& f) noexcept;  // conditional bool

  // A callable type F is Lvalue-Callable for argument types ArgTypes and return
  // type R if the expression INVOKE<R>(declval<F&>(), declval<ArgTypes>()...),
  // considered as an unevaluated operand, is well-formed ([func.require]).

  // f is Cpp17MoveConstructable
  // does not participate in overload unless decay_t<F>& is callable for
  // R(Args...)
  template <class F>
  mofunction(F f);  // false: null ptr, throws if F move constructor does

  mofunction& operator=(mofunction&&);
  mofunction& operator=(nullptr_t) noexcept;

  // f is Cpp17MoveConstructable
  // does not participate in overload unless decay_t<F>& is callable for
  // R(Args...)
  template <class F>
  mofunction& operator=(F&&);
  template <class F>
  mofunction& operator=(reference_wrapper<F>) noexcept;

  mofunction(const mofunction&) = delete;
  mofunction& operator=(const mofunction&) = delete;

  ~mofunction();

  void swap(mofunction&) noexcept;

  explicit operator bool() const noexcept;

  R operator()(ArgTypes... args);

 private:
  void* erased_ = nullptr;
  R (*call_)(void*, ArgTypes...) = nullptr;
  void (*move_constr_)(void*, void*) = nullptr;
  void (*move_assign_)(void*, void*) = nullptr;
  void (*destroy_)(void*) = nullptr;
  void (*delete_)(void*) = nullptr;
};

// template <class R, class... ArgTypes>
// mofunction(R (*)(ArgTypes...))->mofunction<R(ArgTypes...)>;

// Remarks: This deduction guide participates in overload resolution only if
// &F::operator() is well-formed when treated as an unevaluated operand. In that
// case, if decltype(&F::operator()) is of the form R(G::*)(A...) cv &opt
// noexceptopt for a class type G, then the deduced type is mofunction<R(A...)>.
// template <class F>
// mofunction(F)->mofunction<>;

template <class R, class... ArgTypes>
bool operator==(const mofunction<R(ArgTypes...)>&, nullptr_t) noexcept;

template <class R, class... ArgTypes>
bool operator==(nullptr_t, const mofunction<R(ArgTypes...)>&) noexcept;

template <class R, class... ArgTypes>
bool operator!=(const mofunction<R(ArgTypes...)>&, nullptr_t) noexcept;

template <class R, class... ArgTypes>
bool operator!=(nullptr_t, const mofunction<R(ArgTypes...)>&) noexcept;

template <class R, class... ArgTypes>
void swap(mofunction<R(ArgTypes...)>&, mofunction<R(ArgTypes...)>&) noexcept;

template <class R, class... ArgTypes>
mofunction<R(ArgTypes...)>::mofunction() noexcept {}

template <class R, class... ArgTypes>
mofunction<R(ArgTypes...)>::mofunction(nullptr_t) noexcept {}

template <class R, class... ArgTypes>
mofunction<R(ArgTypes...)>::mofunction(mofunction&& f) noexcept {}

template <class R, class... ArgTypes>
template <class F>
mofunction<R(ArgTypes...)>::mofunction(F f)
    : erased_{new F(std::move(f))},
      call_{callable_eraser<R(ArgTypes...)>::template op_call<F>},
      move_constr_{
          callable_eraser<R(ArgTypes...)>::template op_move_construct<F>},
      move_assign_{callable_eraser<R(ArgTypes...)>::template op_move_assign<F>},
      destroy_{callable_eraser<R(ArgTypes...)>::template op_destroy<F>},
      delete_{callable_eraser<R(ArgTypes...)>::template op_delete<F>} {}

template <class R, class... ArgTypes>
mofunction<R(ArgTypes...)>::~mofunction() {
  if (*this) {
    delete_(erased_);
  }
}

template <class R, class... ArgTypes>
mofunction<R(ArgTypes...)>::operator bool() const noexcept {
  return erased_;
}

}  // namespace std

int main() {
  std::cout << "Hello mofo\n";

  {
    std::mofunction<void()> default_constructable;
    assert(!default_constructable);
  }
  {
    std::mofunction<void()> nullptr_constructable(nullptr);
    assert(!nullptr_constructable);
  }
  {
    std::mofunction<void()> f;
    std::mofunction<void()> move_constructable(std::move(f));
    assert(!move_constructable);
  }
  {
    std::mofunction<void()> function_constructable(test_empty_function);
    assert(function_constructable);
  }
  {
    std::mofunction<void()> lambda_constructable(test_empty_lambda);
    assert(lambda_constructable);
  }
  {
    std::mofunction<void()> functor_constructable(TestMoveOnly{});
    assert(functor_constructable);
  }
  {
    std::mofunction<void()> deletes_target(TestDestroy{});
    assert(TestDestroy::destroyed == 1);
  }

  std::cout << "Goodbye!\n";
  return 0;
}
