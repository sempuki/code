#include <cxxabi.h>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

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

  void operator()() const& { std::cout << "TestCopy const call\n"; }
  void operator()() & { std::cout << "TestCopy lref call\n"; }
  void operator()() && { std::cout << "TestCopy rref call\n"; }
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

template <typename Type>
std::string to_type_string() {
  size_t size = 1024;
  char buffer[size];

  int status = 0;
  abi::__cxa_demangle(typeid(Type).name(), buffer, &size, &status);
  assert(status == 0 && "Demanging failed");

  return {buffer, size};
}

class bad_erasure_trait : public std::logic_error {
  using std::logic_error::logic_error;
};

struct basic_eraser {
  using erased_allocate = void* (*)();
  using erased_delete = void (*)(void*);
  using erased_construct = void (*)(void*);
  using erased_destroy = void (*)(void*);

  template <typename Type>
  static void* typed_allocate() {
    return new std::aligned_storage_t<sizeof(Type), alignof(Type)>;
  }

  template <typename Type>
  static void typed_delete(void* target) {
    auto* recovered = reinterpret_cast<Type*>(target);

    delete recovered;
  }

  template <typename Type>
  static void typed_construct(void* target) {
    if constexpr (std::is_default_constructible_v<Type>) {
      auto* recovered = reinterpret_cast<Type*>(target);

      new (recovered) Type();
    } else {
      throw bad_erasure_trait(to_type_string<Type>() +
                              " is not default constructible");
    }
  }

  template <typename Type>
  static void typed_destroy(void* target) {
    if constexpr (std::is_destructible_v<Type>) {
      auto* recovered = reinterpret_cast<Type*>(target);

      recovered->~Type();
    } else {
      throw bad_erasure_trait(to_type_string<Type>() + " is not destructible");
    }
  }
};

struct copy_eraser {
  using erased_construct = void (*)(void*, void*);
  using erased_assign = void (*)(void*, void*);

  template <typename Type>
  static void typed_construct(void* src, void* dst) {
    if constexpr (std::is_copy_constructible_v<Type>) {
      auto* recovered_src = reinterpret_cast<const Type*>(src);
      auto* recovered_dst = reinterpret_cast<Type*>(dst);

      new (recovered_dst) Type(*recovered_src);
    } else {
      throw bad_erasure_trait(to_type_string<Type>() +
                              " is not copy constructible");
    }
  }

  template <typename Type>
  static void typed_assign(void* src, void* dst) {
    if constexpr (std::is_copy_assignable_v<Type>) {
      auto* recovered_src = reinterpret_cast<const Type*>(src);
      auto* recovered_dst = reinterpret_cast<Type*>(dst);

      *recovered_dst = *recovered_src;
    } else {
      throw bad_erasure_trait(to_type_string<Type>() +
                              " is not copy assignable");
    }
  }
};

struct move_eraser {
  using erased_construct = void (*)(void*, void*);
  using erased_assign = void (*)(void*, void*);

  template <typename Type>
  static void typed_construct(void* src, void* dst) {
    if constexpr (std::is_move_constructible_v<Type>) {
      auto* recovered_src = reinterpret_cast<Type*>(src);
      auto* recovered_dst = reinterpret_cast<Type*>(dst);

      new (recovered_dst) Type(std::move(*recovered_src));
    } else {
      throw bad_erasure_trait(to_type_string<Type>() +
                              " is not move constructible");
    }
  }

  template <typename Type>
  static void typed_assign(void* src, void* dst) {
    if constexpr (std::is_move_assignable_v<Type>) {
      auto* recovered_src = reinterpret_cast<Type*>(src);
      auto* recovered_dst = reinterpret_cast<Type*>(dst);

      *recovered_dst = std::move(*recovered_src);
    } else {
      throw bad_erasure_trait(to_type_string<Type>() +
                              " is not move assignable");
    }
  }
};

template <typename Signature>
class call_eraser {};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...)> {
  using erased_call = Return (*)(void*, Args...);

  template <typename Type>
  static Return typed_call(void* target, Args&&... args) {
    if constexpr (std::is_invocable_v<Type, Args...>) {
      auto* recovered = reinterpret_cast<Type*>(target);
      return (*recovered)(std::forward<Args>(args)...);
    } else {
      throw std::bad_function_call(to_type_string<Type>() +
                                   " is not invocable");
    }
  }
};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...) const> {
  using erased_call = Return (*)(const void*, Args...);

  template <typename Type>
  static Return typed_call(const void* target, Args&&... args) {
    if constexpr (std::is_invocable_v<Type const, Args...>) {
      auto* recovered = reinterpret_cast<const Type*>(target);
      return (*recovered)(std::forward<Args>(args)...);
    } else {
      throw std::bad_function_call(to_type_string<Type>() +
                                   " is not const invocable");
    }
  }
};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...) &&> {
  using erased_call = Return (*)(void*, Args...);

  template <typename Type>
  static Return typed_call(void* target, Args&&... args) {
    if constexpr (std::is_invocable_v<Type&&, Args...>) {
      auto* recovered = reinterpret_cast<Type*>(target);
      return std::move(*recovered)(std::forward<Args>(args)...);
    } else {
      throw std::bad_function_call(to_type_string<Type>() +
                                   " is not rv-ref invocable");
    }
  }
};

struct op_allocate {};
struct op_delete {};
struct op_destroy {};
struct op_default_construct {};
struct op_copy_construct {};
struct op_copy_assign {};
struct op_move_construct {};
struct op_move_assign {};
struct op_call {};

template <typename Mixed>
class heap_storage_mixin {
 public:
  template <typename Erased, typename... Args>
  static auto make(Args&&... args) {
    return heap_storage_mixin(new Erased(std::forward<Args>(args)...));
  }

  heap_storage_mixin() = default;

  heap_storage_mixin(const heap_storage_mixin& that)
      : address_(static_cast<Mixed*>(this)->operate(op_allocate{})) {
    static_cast<Mixed*>(this)->operate(op_copy_construct{}, that.address_,
                                       address_);
  }

  heap_storage_mixin& operator=(const heap_storage_mixin& that) {
    if (address_ != that.address_) {
      if (!address_) {
        address_ = static_cast<Mixed*>(this)->operate(op_allocate{});
      }
      static_cast<Mixed*>(this)->operate(op_copy_assign{}, that.address_,
                                         address_);
    }
    return *this;
  }

  heap_storage_mixin(heap_storage_mixin&& that) { swap(that); }

  heap_storage_mixin& operator=(heap_storage_mixin&& that) {
    if (address_ != that.address_) {
      swap(that);
    }
    return *this;
  }

  ~heap_storage_mixin() { release(); }

  void* address() const { return address_; }

  void swap(heap_storage_mixin& that) { std::swap(address_, that.address_); }

  void release() {
    if (address_) {
      static_cast<Mixed*>(this)->operate(op_delete{}, address_);
    }
  }

 private:
  explicit heap_storage_mixin(void* address) : address_(address) {}

  void* address_ = nullptr;
};  // namespace

class compact_dispatch_mixin {
 public:
  template <typename Erased>
  static auto make() {
    return compact_dispatch_mixin(typed_dispatch<Erased>);
  }

  compact_dispatch_mixin() = default;
  ~compact_dispatch_mixin() = default;

  compact_dispatch_mixin(const compact_dispatch_mixin&) = default;
  compact_dispatch_mixin& operator=(const compact_dispatch_mixin&) = default;

  compact_dispatch_mixin(compact_dispatch_mixin&&) = default;
  compact_dispatch_mixin& operator=(compact_dispatch_mixin&&) = default;

  void* operate(op_allocate) {
    return erased_dispatch_(operation::do_allocate, nullptr, nullptr);
  }

  void operate(op_delete, void* object) {
    erased_dispatch_(operation::do_delete, object, nullptr);
  }

  void operate(op_default_construct, void* object) {
    erased_dispatch_(operation::do_default_construct, object, nullptr);
  }

  void operate(op_destroy, void* object) {
    erased_dispatch_(operation::do_destroy, object, nullptr);
  }

  void operate(op_copy_construct, void* src, void* dst) {
    erased_dispatch_(operation::do_copy_construct, src, dst);
  }

  void operate(op_copy_assign, void* src, void* dst) {
    erased_dispatch_(operation::do_copy_assign, src, dst);
  }

  void operate(op_move_construct, void* src, void* dst) {
    erased_dispatch_(operation::do_move_construct, src, dst);
  }

  void operate(op_move_assign, void* src, void* dst) {
    erased_dispatch_(operation::do_move_assign, src, dst);
  }

 private:
  enum class operation {
    do_allocate,
    do_delete,
    do_destroy,
    do_default_construct,
    do_copy_construct,
    do_copy_assign,
    do_move_construct,
    do_move_assign,
  };

  template <typename Type>
  static void* typed_dispatch(operation op, void* lhs, void* rhs) {
    switch (op) {
      case operation::do_allocate:
        return basic_eraser::typed_allocate<Type>();
      case operation::do_delete:
        basic_eraser::typed_delete<Type>(lhs);
        break;
      case operation::do_default_construct:
        basic_eraser::typed_construct<Type>(lhs);
        break;
      case operation::do_destroy:
        basic_eraser::typed_destroy<Type>(lhs);
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
    return nullptr;
  }

  explicit compact_dispatch_mixin(void* (*dispatch)(operation op, void*, void*))
      : erased_dispatch_(dispatch) {}

  void* (*erased_dispatch_)(operation op, void*, void*) = nullptr;
};

template <typename Signature>
class callable_dispatch_mixin {
 public:
  template <typename Erased>
  static auto make() {
    return callable_dispatch_mixin(
        call_eraser<Signature>::template typed_call<Erased>);
  }

  callable_dispatch_mixin() = default;
  ~callable_dispatch_mixin() = default;

  callable_dispatch_mixin(const callable_dispatch_mixin&) = default;
  callable_dispatch_mixin& operator=(const callable_dispatch_mixin&) = default;

  callable_dispatch_mixin(callable_dispatch_mixin&&) = default;
  callable_dispatch_mixin& operator=(callable_dispatch_mixin&&) = default;

  template <typename Return, typename... Args>
  Return operate(op_call, void* target, Args&&... args) {
    return erased_call_(target, std::forward<Args>(args)...);
  }

  template <typename Return, typename... Args>
  Return operate(op_call, void* target, Args&&... args) const {
    return erased_call_(target, std::forward<Args>(args)...);
  }

 private:
  explicit callable_dispatch_mixin(
      typename call_eraser<Signature>::erased_call call)
      : erased_call_(call) {}

  typename call_eraser<Signature>::erased_call erased_call_ = nullptr;
};

template <typename Signature>
class callable_compact_dispatch_mixin
    : public callable_dispatch_mixin<Signature>,
      compact_dispatch_mixin {
 public:
  template <typename Erased>
  static auto make() {
    return callable_compact_dispatch_mixin(
        callable_dispatch_mixin<Signature>::template make<Erased>(),
        compact_dispatch_mixin::template make<Erased>());
  }

  callable_compact_dispatch_mixin() = default;
  ~callable_compact_dispatch_mixin() = default;

  callable_compact_dispatch_mixin(const callable_compact_dispatch_mixin&) =
      default;
  callable_compact_dispatch_mixin& operator=(
      const callable_compact_dispatch_mixin&) = default;

  callable_compact_dispatch_mixin(callable_compact_dispatch_mixin&&) = default;
  callable_compact_dispatch_mixin& operator=(
      callable_compact_dispatch_mixin&&) = default;

  using callable_dispatch_mixin<Signature>::operate;
  using compact_dispatch_mixin::operate;

 private:
  explicit callable_compact_dispatch_mixin(
      callable_dispatch_mixin<Signature> callable,
      compact_dispatch_mixin compact)
      : callable_dispatch_mixin<Signature>(std::move(callable)),
        compact_dispatch_mixin(std::move(compact)) {}
};

template <typename Mixed, typename Signature>
class callable_interface_mixin {};

template <typename Mixed, typename Return, typename... Args>
struct callable_interface_mixin<Mixed, Return(Args...) const> {
  Return operator()(Args&&... args) const& {
    return static_cast<const Mixed*>(this)->template operate<Return, Args...>(
        op_call{}, static_cast<const Mixed*>(this)->address(),
        std::forward<Args>(args)...);
  }
};

template <typename Mixed, typename Return, typename... Args>
struct callable_interface_mixin<Mixed, Return(Args...)> {
  Return operator()(Args&&... args) & {
    return static_cast<Mixed*>(this)->template operate<Return, Args...>(
        op_call{}, static_cast<Mixed*>(this)->address(),
        std::forward<Args>(args)...);
  }
};

template <typename Mixed, typename Return, typename... Args>
struct callable_interface_mixin<Mixed, Return(Args...) &&> {
  Return operator()(Args&&... args) && {
    return static_cast<Mixed*>(this)->template operate<Return, Args...>(
        op_call{}, static_cast<Mixed*>(this)->address(),
        std::forward<Args>(args)...);
  }
};

}  // namespace

namespace std {
template <class Signature>
class mofunction {};

template <class R, class... As>
class mofunction<R(As...)> : private callable_compact_dispatch_mixin<R(As...)>,
                             heap_storage_mixin<mofunction<R(As...)>> {
 private:
  using dispatch_mixin_type = callable_compact_dispatch_mixin<R(As...)>;
  using storage_mixin_type = heap_storage_mixin<mofunction<R(As...)>>;

  friend dispatch_mixin_type;
  friend storage_mixin_type;

 public:
  using result_type = R;

  mofunction() noexcept = default;
  mofunction(nullptr_t) noexcept : mofunction() {}

  mofunction(mofunction&& f) noexcept
      : dispatch_mixin_type(static_cast<dispatch_mixin_type&&>(f)),
        storage_mixin_type(static_cast<storage_mixin_type&&>(f)) {}

  // A callable type F is Lvalue-Callable for argument types As and return
  // type R if the expression INVOKE<R>(declval<F&>(), declval<As>()...),
  // considered as an unevaluated operand, is well-formed ([func.require]).

  // Requires: f is Cpp17MoveConstructable
  // Remarks: does not participate in overload unless decay_t<F>& is
  // callable for R(Args...)

  template <class F, typename std::enable_if_t<
                         std::is_invocable_v<std::decay_t<F>, As...>, int> = 0>
  mofunction(F&& f)
      : dispatch_mixin_type(
            dispatch_mixin_type::template make<std::decay_t<F>>()),
        storage_mixin_type(storage_mixin_type::template make<std::decay_t<F>>(
            std::forward<F>(f))) {}

  mofunction& operator=(mofunction&& f) {
    if (this != &f) {
      *this = static_cast<dispatch_mixin_type&&>(f);
      *this = static_cast<storage_mixin_type&&>(f);
    }
    return *this;
  }

  mofunction& operator=(nullptr_t) noexcept { storage_mixin_type::release(); }

  template <class F, typename std::enable_if_t<
                         std::is_invocable_v<std::decay_t<F>, As...>, int> = 0>
  mofunction& operator=(F&& f) {
    *this = dispatch_mixin_type::template make<std::decay_t<F>>();
    *this =
        storage_mixin_type::template make<std::decay_t<F>>(std::forward<F>(f));
    return *this;
  }

  mofunction(const mofunction&) = delete;
  mofunction& operator=(const mofunction&) = delete;

  ~mofunction() = default;

  void swap(mofunction& f) noexcept { storage_mixin_type::swap(f); }

  explicit operator bool() const noexcept {
    return storage_mixin_type::address() != nullptr;
  }

};  // namespace std

template <class R, class... As>
class mofunction<R(As...) const> {};

template <class R, class... As>
class mofunction<R(As...) &&> {};

/*
// template <class R, class... As>
// mofunction(R (*)(As...))->mofunction<R(As...)>;

// Remarks: This deduction guide participates in overload resolution only if
// &F::operator() is well-formed when treated as an unevaluated operand. In
that
// case, if decltype(&F::operator()) is of the form R(G::*)(A...) cv &opt
// noexceptopt for a class type G, then the deduced type is
mofunction<R(A...)>.
// template <class F>
// mofunction(F)->mofunction<>;

template <class R, class... As>
bool operator==(const mofunction<R(As...)>&, nullptr_t) noexcept;

template <class R, class... As>
bool operator==(nullptr_t, const mofunction<R(As...)>&) noexcept;

template <class R, class... As>
bool operator!=(const mofunction<R(As...)>&, nullptr_t) noexcept;

template <class R, class... As>
bool operator!=(nullptr_t, const mofunction<R(As...)>&) noexcept;

template <class R, class... As>
void swap(mofunction<R(As...)>&, mofunction<R(As...)>&) noexcept;

template <class R, class... As>
mofunction<R(As...)>::mofunction() noexcept {}

template <class R, class... As>
mofunction<R(As...)>::mofunction(nullptr_t) noexcept {}

template <class R, class... As>
mofunction<R(As...)>::mofunction(mofunction&& f) noexcept {}

template <class R, class... As>
template <class F>
mofunction<R(As...)>::mofunction(F f) {}

template <class R, class... As>
mofunction<R(As...)>::operator bool() const noexcept {
  return erased_;
}
*/
}  // namespace std

int main() {
  std::cout << "Hello mofo\n";

  try {
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
  } catch (std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << "\n";
    assert(false);
  }

  std::cout << "Goodbye!\n";
  return 0;
}

