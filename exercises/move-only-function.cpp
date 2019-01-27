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
  TestCopy() { std::cout << "TestCopy default construct\n"; }
  ~TestCopy() { std::cout << "TestCopy deconstruct\n"; }

  TestCopy(const TestCopy&) { std::cout << "TestCopy copy construct\n"; }
  TestCopy& operator=(const TestCopy&) {
    std::cout << "TestCopy copy assign\n";
    return *this;
  }

  TestCopy(TestCopy&&) { std::cout << "TestCopy move construct\n"; }
  TestCopy& operator=(TestCopy&&) {
    std::cout << "TestCopy move assign\n";
    return *this;
  }

  void operator()() { std::cout << "TestCopy call\n"; }
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

struct basic_eraser {
  using erased_delete = void (*)(void*);

  template <typename Type>
  static void typed_delete(void* target) {
    auto* recovered = reinterpret_cast<Type*>(target);

    delete recovered;
  }
};

struct default_eraser {
  using erased_construct = void (*)(void*);

  template <typename Type>
  static void typed_construct(void* target) {
    auto* recovered = reinterpret_cast<Type*>(target);

    new (recovered) Type();
  }
};

struct copy_eraser {
  using erased_construct = void (*)(const void*, void*);
  using erased_assign = void (*)(const void*, void*);

  template <typename Type>
  static void typed_construct(const void* src, void* dst) {
    auto* recovered_src = reinterpret_cast<const Type*>(src);
    auto* recovered_dst = reinterpret_cast<Type*>(dst);

    new (recovered_dst) Type(*recovered_src);
  }

  template <typename Type>
  static void typed_assign(const void* src, void* dst) {
    auto* recovered_src = reinterpret_cast<const Type*>(src);
    auto* recovered_dst = reinterpret_cast<Type*>(dst);

    *recovered_dst = *recovered_src;
  }
};

struct move_eraser {
  using erased_construct = void (*)(void*, void*);
  using erased_assign = void (*)(void*, void*);

  template <typename Type>
  static void typed_construct(void* src, void* dst) {
    auto* recovered_src = reinterpret_cast<Type*>(src);
    auto* recovered_dst = reinterpret_cast<Type*>(dst);

    new (recovered_dst) Type(std::move(*recovered_src));
  }

  template <typename Type>
  static void typed_assign(void* src, void* dst) {
    auto* recovered_src = reinterpret_cast<Type*>(src);
    auto* recovered_dst = reinterpret_cast<Type*>(dst);

    *recovered_dst = std::move(*recovered_src);
  }
};

template <typename Signature>
class call_eraser {};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...)> {
  using erased_call = Return (*)(void* target, Args&&... args);

  template <typename Type>
  static Return typed_call(void* target, Args&&... args) {
    auto* recovered = reinterpret_cast<Type*>(target);
    return (*recovered)(std::forward<Args>(args)...);
  }
};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...) const> {
  using erased_call = Return (*)(const void* target, Args&&... args);

  template <typename Type>
  static Return typed_call(const void* target, Args&&... args) {
    auto* recovered = reinterpret_cast<const Type*>(target);
    return (*recovered)(std::forward<Args>(args)...);
  }
};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...) &&> {
  using erased_call = Return (*)(void* target, Args&&... args);

  template <typename Type>
  static Return typed_call(void* target, Args&&... args) {
    auto* recovered = reinterpret_cast<Type*>(target);
    return std::move(*recovered)(std::forward<Args>(args)...);
  }
};

struct op_delete {};
struct op_copy_construct {};
struct op_copy_assign {};
struct op_move_construct {};
struct op_move_assign {};
struct op_call {};

template <typename Delegate>
class heap_erasure {
 public:
  template <typename Erased>
  heap_erasure(Erased&& object)
      : object_{new Erased(std::forward<Erased>(object))} {}

  ~heap_erasure() {
    static_cast<Delegate*>(this)->operate(op_delete{}, address());
  }

  const void* address() const { return object_; }
  void* address() { return object_; }

 private:
  void* object_ = nullptr;
};

template <typename Delegate>
class copyable_erasure {
 public:
  copyable_erasure() = default;

  copyable_erasure(const Delegate& that) {
    operate(op_copy_construct{}, that.address(),
            static_cast<Delegate*>(this)->address());
  }

  Delegate& operator=(const Delegate& that) {
    operate(op_copy_construct{}, that.address(),
            static_cast<Delegate*>(this)->address());
    return *this;
  }
};

template <typename Delegate>
class movable_erasure {
 public:
  movable_erasure() = default;

  movable_erasure(Delegate&& that) {
    operate(op_move_construct{}, that.address(),
            static_cast<Delegate*>(this)->address());
  }

  Delegate& operator=(Delegate&& that) {
    operate(op_move_construct{}, that.address(),
            static_cast<Delegate*>(this)->address());
    return *this;
  }
};

template <typename Delegate>
class callable_erasure {
 public:
  callable_erasure() = default;

  template <typename Return, typename... Args>
  Return operator()(Args&&... args) {
    operate(op_call{}, static_cast<Delegate*>(this)->address(),
            std::forward<Args>(args)...);
  }
};

// TODO: const_callable_erasure, move_callable_erasure

struct compact_dispatcher {
  enum class operation {
    do_delete,
    do_copy_construct,
    do_copy_assign,
    do_move_construct,
    do_move_assign,
  };

  template <typename Type>
  static void typed_dispatch(operation op, void* lhs, void* rhs) {
    switch (op) {
      case operation::do_delete:
        basic_eraser::typed_delete<Type>(lhs);
        break;
      case operation::do_copy_construct:
        copy_eraser::typed_construct<Type>(lhs, rhs);
        break;
      case operation::do_copy_assign:
        copy_eraser::typed_assign<Type>(lhs, rhs);
        break;
      case operation::do_move_construct:
        move_eraser::typed_construct<Type>(lhs, rhs);
        break;
      case operation::do_move_assign:
        move_eraser::typed_assign<Type>(lhs, rhs);
        break;
    }
  }

  template <typename Erased>
  static compact_dispatcher make() {
    return {typed_dispatch<Erased>};
  }

  void operate(struct op_delete, void* object) {
    erased_dispatch(operation::do_delete, object, nullptr);
  }

  void operate(struct op_copy_construct, void* src, void* dst) {
    erased_dispatch(operation::do_copy_construct, src, dst);
  }

  void operate(struct op_copy_assign, void* src, void* dst) {
    erased_dispatch(operation::do_copy_assign, src, dst);
  }

  void operate(struct op_move_construct, void* src, void* dst) {
    erased_dispatch(operation::do_move_construct, src, dst);
  }

  void operate(struct op_move_assign, void* src, void* dst) {
    erased_dispatch(operation::do_move_assign, src, dst);
  }

  void (*erased_dispatch)(operation op, void*, void*) = nullptr;
};

template <typename Signature>
struct callabe_compact_dispatcher {};

template <typename Return, typename... Args>
struct callabe_compact_dispatcher<Return(Args...)> : public compact_dispatcher {
  using compact_dispatcher::operate;

  Return operate(struct op_call, void* object, Args&&... args) {
    return erased_call(object, std::forward<Args>(args)...);
  }

  typename call_eraser<Return(Args...)>::erased_call erased_call = nullptr;
};

}  // namespace

/*
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
      call_{call_eraser<R(ArgTypes...)>::template call<F>},
      move_constr_{move_eraser::template construct<F>},
      move_assign_{move_eraser::template assign<F>},
      destroy_{default_eraser::template destroy<F>},

      template <class R, class... ArgTypes>
      mofunction<R(ArgTypes...)>::~mofunction() {}

template <class R, class... ArgTypes>
mofunction<R(ArgTypes...)>::operator bool() const noexcept {
  return erased_;
}

}  // namespace std
*/

struct MyErasure : public compact_dispatcher,
                   heap_erasure<MyErasure>,
                   copyable_erasure<MyErasure> {
  template <typename Erased>
  MyErasure(Erased&& object)
      : compact_dispatcher{compact_dispatcher::make<Erased>()},
        heap_erasure<MyErasure>{std::forward<Erased>(object)} {}
};

int main() {
  std::cout << "Hello mofo\n";

  MyErasure a{TestCopy{}};
  MyErasure b = a;

  /*
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
  */

  std::cout << "Goodbye!\n";
  return 0;
}
