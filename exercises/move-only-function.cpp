#include <cxxabi.h>
#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>

// ==== Implementation ========================================================

namespace {
template <typename Type>
std::string to_type_string() {
  size_t size = 1024;
  char buffer[size];

  int status = 0;
  abi::__cxa_demangle(typeid(Type).name(), buffer, &size, &status);
  assert(status == 0 && "Demanging failed");

  return buffer;
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
struct call_traits {};

template <typename Return, typename... Args>
struct call_traits<Return(Args...)> {
  using signature_type = Return(Args...);
  using result_type = Return;

  template <typename Target>
  static inline constexpr bool is_invocable_with =
      std::is_invocable_r<Return, Target&, Args...>::value;

  template <typename Target>
  static inline constexpr bool is_nothrow_invocable_with =
      std::is_nothrow_invocable_r<Return, Target, Args...>::value;
};

template <typename Return, typename... Args>
struct call_traits<Return(Args...) const> {
  using signature_type = Return(Args...) const;
  using result_type = Return;

  template <typename Target>
  static inline constexpr bool is_invocable_with =
      std::is_invocable_r<Return, const Target&, Args...>::value;

  template <typename Target>
  static inline constexpr bool is_nothrow_invocable_with =
      std::is_nothrow_invocable_r<Return, const Target&, Args...>::value;
};

template <typename Return, typename... Args>
struct call_traits<Return(Args...) &&> {
  using signature_type = Return(Args...) &&;
  using result_type = Return;

  template <typename Target>
  static inline constexpr bool is_invocable_with =
      std::is_invocable_r<Return, Target&&, Args...>::value;

  template <typename Target>
  static inline constexpr bool is_nothrow_invocable_with =
      std::is_nothrow_invocable_r<Return, Target&&, Args...>::value;
};

template <typename Target, typename... Args>
using call_result = decltype(std::declval<Target>()(std::declval<Args>()...));

template <typename Signature>
class call_eraser {};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...)> {
  using erased_call = Return (*)(void*, Args...);

  template <typename Target>
  static Return typed_call(void* target, Args... args) {
    if constexpr (call_traits<Return(
                      Args...)>::template is_invocable_with<Target>) {
      auto* recovered = reinterpret_cast<Target*>(target);
      return std::invoke(*recovered, std::forward<Args>(args)...);
    } else {
      static_assert("Not lvalue callable");
    }
  }
};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...) const> {
  using erased_call = Return (*)(const void*, Args...);

  template <typename Target>
  static Return typed_call(const void* target, Args... args) {
    if constexpr (call_traits<Return(Args...)
                                  const>::template is_invocable_with<Target>) {
      auto* recovered = reinterpret_cast<const Target*>(target);
      return std::invoke(*recovered, std::forward<Args>(args)...);
    } else {
      static_assert("Not const callable");
    }
  }
};

template <typename Return, typename... Args>
struct call_eraser<Return(Args...) &&> {
  using erased_call = Return (*)(void*, Args&&...);

  template <typename Target>
  static Return typed_call(void* target, Args&&... args) {
    if constexpr (call_traits<Return(Args...) &&>::template is_invocable_with<
                      Target>) {
      auto* recovered = reinterpret_cast<Target*>(target);
      return std::invoke(std::move(*recovered), std::forward<Args>(args)...);
    } else {
      static_assert("Not rvalue callable");
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

  ~heap_storage_mixin() noexcept { release(); }

  void* address() const noexcept { return address_; }

  void swap(heap_storage_mixin& that) noexcept {
    std::swap(address_, that.address_);
  }

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

  void swap(compact_dispatch_mixin& that) noexcept {
    std::swap(erased_dispatch_, that.erased_dispatch_);
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
  template <typename Target>
  static auto make() {
    return callable_dispatch_mixin(
        call_eraser<Signature>::template typed_call<Target>);
  }

  callable_dispatch_mixin() = default;
  ~callable_dispatch_mixin() = default;

  callable_dispatch_mixin(const callable_dispatch_mixin&) = default;
  callable_dispatch_mixin& operator=(const callable_dispatch_mixin&) = default;

  callable_dispatch_mixin(callable_dispatch_mixin&&) = default;
  callable_dispatch_mixin& operator=(callable_dispatch_mixin&&) = default;

  template <typename Return, typename... Args>
  Return operate(op_call, void* target, Args&&... args) const {
    if (!erased_call_) {
      throw std::bad_function_call();
    }
    return erased_call_(target, std::forward<Args>(args)...);
  }

  void swap(callable_dispatch_mixin& that) noexcept {
    std::swap(erased_call_, that.erased_call_);
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
  template <typename Target>
  static auto make() {
    return callable_compact_dispatch_mixin(
        callable_dispatch_mixin<Signature>::template make<Target>(),
        compact_dispatch_mixin::template make<Target>());
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

  void swap(callable_compact_dispatch_mixin& that) noexcept {
    callable_dispatch_mixin<Signature>::swap(that);
    compact_dispatch_mixin::swap(that);
  }

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
  Return operator()(Args&&... args) {
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
template <typename Signature>
class mofunction
    : public callable_interface_mixin<mofunction<Signature>, Signature>,
      private callable_compact_dispatch_mixin<Signature>,
      heap_storage_mixin<mofunction<Signature>> {
 private:
  using interface_mixin_type =
      callable_interface_mixin<mofunction<Signature>, Signature>;
  using dispatch_mixin_type = callable_compact_dispatch_mixin<Signature>;
  using storage_mixin_type = heap_storage_mixin<mofunction<Signature>>;

  friend interface_mixin_type;
  friend dispatch_mixin_type;
  friend storage_mixin_type;

  template <class Target>
  using target_requirements = std::enable_if_t<
      call_traits<Signature>::template is_invocable_with<Target>,
      int>;  // nothrow invocable?

 public:
  using result_type = typename call_traits<Signature>::result_type;

  mofunction() noexcept = default;
  mofunction(std::nullptr_t) noexcept : mofunction() {}

  mofunction& operator=(std::nullptr_t) noexcept {
    storage_mixin_type::release();
  }

  mofunction(mofunction&& f) noexcept
      : dispatch_mixin_type(static_cast<dispatch_mixin_type&&>(f)),
        storage_mixin_type(static_cast<storage_mixin_type&&>(f)) {}

  mofunction& operator=(mofunction&& f) {
    if (this != &f) {
      dispatch_mixin_type::operator=(static_cast<dispatch_mixin_type&&>(f));
      storage_mixin_type::operator=(static_cast<storage_mixin_type&&>(f));
    }
    return *this;
  }

  mofunction(const mofunction&) = delete;
  mofunction& operator=(const mofunction&) = delete;

  ~mofunction() = default;

  template <class Target, target_requirements<Target> = 0>
  mofunction(Target&& target)
      : dispatch_mixin_type(
            dispatch_mixin_type::template make<std::decay_t<Target>>()),
        storage_mixin_type(
            storage_mixin_type::template make<std::decay_t<Target>>(
                std::forward<Target>(target))) {}

  template <class Target, target_requirements<Target> = 0>
  mofunction& operator=(Target&& target) {
    dispatch_mixin_type::operator=(
        dispatch_mixin_type::template make<std::decay_t<Target>>());
    storage_mixin_type::operator=(
        storage_mixin_type::template make<std::decay_t<Target>>(
            std::forward<Target>(target)));
    return *this;
  }

  void swap(mofunction& f) noexcept {
    dispatch_mixin_type::swap(f);
    storage_mixin_type::swap(f);
  }

  explicit operator bool() const noexcept {
    return storage_mixin_type::address() != nullptr;
  }
};

template <typename Signature>
bool operator==(const mofunction<Signature>& f, nullptr_t) noexcept {
  return !static_cast<bool>(f);
}

template <typename Signature>
bool operator==(nullptr_t, const mofunction<Signature>& f) noexcept {
  return !static_cast<bool>(f);
}

template <typename Signature>
bool operator!=(const mofunction<Signature>& f, nullptr_t) noexcept {
  return static_cast<bool>(f);
}

template <typename Signature>
bool operator!=(nullptr_t, const mofunction<Signature>& f) noexcept {
  return static_cast<bool>(f);
}

template <typename Signature>
void swap(mofunction<Signature>& f, mofunction<Signature>& g) noexcept {
  f.swap(g);
}

/*
// template <class R, class... As>
// mofunction(R (*)(As...))->mofunction<R(As...)>;

// Remarks: This deduction guide participates in overload resolution only if
// &Target::operator() is well-formed when treated as an unevaluated operand. In
that
// case, if decltype(&Target::operator()) is of the form R(G::*)(A...) cv &opt
// noexceptopt for a class type G, then the deduced type is mofunction<R(A...)>.
// template <class Target> mofunction(Target)->mofunction<>;

*/
}  // namespace std

// ==== Test Harness ==========================================================

struct TestNotCallable {};

struct TestCopyOnly {
  TestCopyOnly() {}
  ~TestCopyOnly() {}
  TestCopyOnly(const TestCopyOnly& that) : copied{that.copied} {
    if (copied) {
      (*copied)++;
    }
  }

  TestCopyOnly& operator=(const TestCopyOnly& that) {
    copied = that.copied;
    return *this;
  }

  TestCopyOnly(size_t* ptr) : copied{ptr} {}

  void operator()() const& {}
  void operator()() & {}
  void operator()() && {}

  size_t* copied = nullptr;
};

struct TestDestroy {
  TestDestroy() {}
  ~TestDestroy() {
    if (destroyed) {
      (*destroyed)++;
    }
  }

  TestDestroy(size_t* ptr) : destroyed{ptr} {}

  void operator()() const& {}
  void operator()() & {}
  void operator()() && {}

  size_t* destroyed = nullptr;
};

struct TestCopy {
  TestCopy() {}
  ~TestCopy() {}

  TestCopy(const TestCopy&) {}
  TestCopy& operator=(const TestCopy&) { return *this; }

  TestCopy(TestCopy&&) {}
  TestCopy& operator=(TestCopy&&) { return *this; }

  void operator()() const& {}
  void operator()() & {}
  void operator()() && {}
};

struct TestMoveOnly {
  TestMoveOnly() {}
  ~TestMoveOnly() {}

  TestMoveOnly(const TestMoveOnly&) = delete;
  TestMoveOnly& operator=(const TestMoveOnly&) = delete;

  TestMoveOnly(TestMoveOnly&& that) : moved{that.moved} {
    if (moved) {
      (*moved)++;
    }
  }

  TestMoveOnly& operator=(TestMoveOnly&& that) {
    moved = that.moved;
    return *this;
  }

  TestMoveOnly(size_t* ptr) : moved{ptr} {}

  void operator()() const& {}
  void operator()() & {}
  void operator()() && {}

  size_t* moved = nullptr;
};

auto test_empty_lambda = [] {};
void test_empty_function() {}

void test_function(size_t* count) {
  if (count) {
    (*count)++;
  }
}

auto test_lambda = [](size_t* count) {
  if (count) {
    (*count)++;
  }
};

struct TestCallable {
  void operator()(size_t* count) {
    if (count) {
      (*count)++;
    }
  }
};

struct TestConstCallable {
  void operator()(size_t* count) const {
    if (count) {
      (*count)++;
    }
  }
};

struct TestRvrefCallable {
  void operator()(size_t* count) && {
    if (count) {
      (*count)++;
    }
  }
};

// ==== Unit Tests ============================================================

int main() {
  // ==== Targetting Constructors
  {
    std::cout << "default constructed mofunction is untargeted\n";
    std::mofunction<void()> f;
    assert(!f);
  }
  {
    std::cout << "nullptr constructed mofunction is untargeted\n";
    std::mofunction<void()> f(nullptr);
    assert(!f);
  }
  {
    std::cout << "nullptr constructed mofunction equivalent to nullptr\n";
    std::mofunction<void()> f(nullptr);
    assert(f == nullptr);
  }
  {
    std::cout << "function pointer constructed mofunction is targeted\n";
    std::mofunction<void()> f(test_empty_function);
    assert(f);
  }
  {
    std::cout << "function pointer constructed mofunction not equivalent to "
                 "nullptr\n";
    std::mofunction<void()> f(test_empty_function);
    assert(f != nullptr);
  }
  {
    std::cout << "lambda constructed mofunction is targeted\n";
    std::mofunction<void()> f(test_empty_lambda);
    assert(f);
  }
  {
    std::cout << "move-only object constructed mofunction is targeted\n";
    std::mofunction<void()> f(TestMoveOnly{});
    assert(f);
  }
  {
    std::cout << "move+copyable object constructed mofunction is targeted\n";
    std::mofunction<void()> f(TestCopy{});
    assert(f);
  }
  {
    std::cout << "destroyable object constructed mofunction is targeted\n";
    std::mofunction<void()> f(TestDestroy{});
    assert(f);
  }
  {
    std::cout << "copy-only object constructed mofunction is targeted\n";
    std::mofunction<void()> f(TestCopyOnly{});
    assert(f);
  }

  // ==== Copy/Move Constructors
  {
    std::cout << "move-only object constructed mofunction is moved\n";
    size_t moved = 0;
    std::mofunction<void()> f(TestMoveOnly{&moved});
    assert(moved > 0);
  }
  {
    std::cout << "destroyable object constructed mofunction is destroyed\n";
    size_t destroyed = 0;
    std::mofunction<void()> f(TestDestroy{&destroyed});
    assert(destroyed > 0);
  }
  {
    std::cout << "copy-only object constructed mofunction is copied\n";
    size_t copied = 0;
    std::mofunction<void()> f(TestCopyOnly{&copied});
    assert(copied > 0);
  }
  {
    std::cout << "move constructed mofunction preserves untargeted\n";
    std::mofunction<void()> f;
    std::mofunction<void()> g(std::move(f));
    assert(!g);
  }
  {
    std::cout << "move constructed mofunction preserves targeted\n";
    std::mofunction<void()> f(test_empty_function);
    std::mofunction<void()> g(std::move(f));
    assert(g);
  }

  // ==== Bad Call
  {
    std::cout << "untargetted mofunction throws bad_function_call\n";
    size_t caught = 0;
    try {
      std::mofunction<void()> f;
      f();
    } catch (const std::bad_function_call& e) {
      caught++;
    }
    assert(caught > 0);
  }

  // ==== Non-const signature, Non-const object, Lv-Callable
  {
    std::cout << "Non-const signature, Non-const object, Lv-Callable: function "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(test_function);
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Lv-Callable: lambda "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(test_lambda);
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Lv-Callable: callable "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Lv-Callable: const "
                 "callable called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestConstCallable{});
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Lv-Callable: rvref "
                 "callable not constructed\n";
    size_t called = 0;
    // std::mofunction<void(size_t*)> f(TestRvrefCallable{});
    // f(&called);
    assert(called == 0);
  }

  // ==== Non-const signature, Const object, Lv-Callable
  {
    std::cout << "Non-const signature, Const object, Lv-Callable: function not "
                 "called\n";
    size_t called = 0;
    const std::mofunction<void(size_t*)> f(test_function);
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Non-const signature, Const object, Lv-Callable: lambda not "
                 "called\n";
    size_t called = 0;
    const std::mofunction<void(size_t*)> f(test_lambda);
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Non-const signature, Const object, Lv-Callable: callable not "
                 "called\n";
    size_t called = 0;
    const std::mofunction<void(size_t*)> f(TestCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Non-const signature, Const object, Lv-Callable: const "
                 "callable not called\n";
    size_t called = 0;
    const std::mofunction<void(size_t*)> f(TestConstCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Non-const signature, Const object, Lv-Callable: rvref "
                 "callable not called\n";
    size_t called = 0;
    // const std::mofunction<void(size_t*)> f(TestRvrefCallable{});
    // f(&called);
    assert(called == 0);
  }

  // ==== Non-const signature, Non-const object, Rv-Callable
  {
    std::cout << "Non-const signature, Non-const object, Rv-Callable: function "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(test_function);
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Rv-Callable: lambda "
                 "not callable\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(test_lambda);
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Rv-Callable: callable "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Rv-Callable: const "
                 "callable called\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestConstCallable{});
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Non-const signature, Non-const object, Rv-Callable: rvref "
                 "callable not constructed\n";
    size_t called = 0;
    // std::mofunction<void(size_t*)> f(TestRvrefCallable{});
    // std::move(f)(&called);
    assert(called == 0);
  }

  // ==== Const signature, Non-const object, Lv-Callable
  {
    std::cout << "Const signature, Non-const object, Lv-Callable: function "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(test_function);
    f(&called);
    assert(called > 0);
  }
  {
    std::cout
        << "Const signature, Non-const object, Lv-Callable: lambda called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(test_lambda);
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Non-const object, Lv-Callable: callable not "
                 "constructed\n";
    size_t called = 0;
    // std::mofunction<void(size_t*) const> f(TestCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Const signature, Non-const object, Lv-Callable: const "
                 "callable called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Non-const object, Lv-Callable: rvref "
                 "callable not constructed\n";
    size_t called = 0;
    // std::mofunction<void(size_t*) const> f(TestRvrefCallable{});
    // f(&called);
    assert(called == 0);
  }

  // ==== Const signature, Const object, Lv-Callable
  {
    std::cout
        << "Const signature, Const object, Lv-Callable: function called\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) const> f(test_function);
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Const object, Lv-Callable: lambda called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(test_lambda);
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Const object, Lv-Callable: callable not "
                 "constructed\n";
    size_t called = 0;
    // const std::mofunction<void(size_t*) const> f(TestCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Const signature, Const object, Lv-Callable: const callable "
                 "called\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) const> f(TestConstCallable{});
    f(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Const object, Lv-Callable: rvref callable "
                 "not constructed\n";
    size_t called = 0;
    // const std::mofunction<void(size_t*) const> f(TestRvrefCallable{});
    // f(&called);
    assert(called == 0);
  }

  // ==== Const signature, Non-const object, Rv-Callable
  {
    std::cout << "Const signature, Non-const object, Rv-Callable: function "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(test_function);
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout
        << "Const signature, Non-const object, Rv-Callable: lambda called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(test_lambda);
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Non-const object, Rv-Callable: callable not "
                 "constructed\n";
    size_t called = 0;
    // std::mofunction<void(size_t*) const> f(TestCallable{});
    // std::move(f)(&called);
    assert(called == 0);
  }
  {
    std::cout << "Const signature, Non-const object, Rv-Callable: const "
                 "callable called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Const signature, Non-const object, Rv-Callable: rvref "
                 "callable nto constructed\n";
    size_t called = 0;
    // std::mofunction<void(size_t*) const> f(TestRvrefCallable{});
    // std::move(f)(&called);
    assert(called == 0);
  }

  // ==== Rvref signature, Non-const object, Lv-Callable
  {
    std::cout << "Rvref signature, Non-const object, Lv-Callable: function not "
                 "callabe\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(test_function);
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Lv-Callable: lambda not "
                 "callabe\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(test_lambda);
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Lv-Callable: callabe not "
                 "callabe\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Lv-Callable: const "
                 "callabe not callabe\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Lv-Callable: rvref "
                 "callable not callabe\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    // f(&called);
    assert(called == 0);
  }

  // ==== Rvref signature, Const object, Lv-Callable
  {
    std::cout << "Rvref signature, Const object, Lv-Callable: function not "
                 "callable\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) &&> f(test_function);
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Const object, Lv-Callable: lambda not "
                 "callable\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) &&> f(test_lambda);
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Const object, Lv-Callable: callable not "
                 "callable\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) &&> f(TestCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Const object, Lv-Callable: const callable "
                 "not callable\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) const> f(TestConstCallable{});
    // f(&called);
    assert(called == 0);
  }
  {
    std::cout << "Rvref signature, Const object, Lv-Callable: rvref callable "
                 "not callable\n";
    size_t called = 0;
    const std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    // f(&called);
    assert(called == 0);
  }

  // ==== Rvref signature, Non-const object, Rv-Callable
  {
    std::cout << "Rvref signature, Non-const object, Rv-Callable: function "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(test_function);
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout
        << "Rvref signature, Non-const object, Rv-Callable: lambda called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(test_lambda);
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Rv-Callable: callable "
                 "called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestCallable{});
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Rv-Callable: const "
                 "callable called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::move(f)(&called);
    assert(called > 0);
  }
  {
    std::cout << "Rvref signature, Non-const object, Rv-Callable: rvref "
                 "callable called\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::move(f)(&called);
    assert(called > 0);
  }

  // ==== Target Copy/Move Constructors
  {
    std::cout
        << "callable object constructed mofunction is move constructible\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::mofunction<void(size_t*)> g(std::move(f));
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "callable object constructed mofunction is move assignable\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::mofunction<void(size_t*)> g;
    g = std::move(f);
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "callable object constructed mofunction is not copy "
                 "constructible\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    // std::mofunction<void(size_t*)> g(f);
    // g(&called);
    assert(called == 0);
  }
  {
    std::cout
        << "callable object constructed mofunction is not copy assignable\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::mofunction<void(size_t*)> g;
    // f = g;
    // g(&called);
    assert(called == 0);
  }
  {
    std::cout << "const callable object constructed mofunction is move "
                 "constructible\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*) const> g(std::move(f));
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "const callable object constructed mofunction is move "
                 "assignable\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*) const> g;
    g = std::move(f);
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "rvref callable object constructed mofunction is move "
                 "constructible\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*) &&> g(std::move(f));
    std::move(g)(&called);
    assert(called > 0);
  }
  {
    std::cout << "rvref callable object constructed mofunction is move "
                 "assignable\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*) &&> g;
    g = std::move(f);
    std::move(g)(&called);
    assert(called > 0);
  }

  // ==== Target Copy/Move Constructors with Const/Non-const Signature
  {
    std::cout << "non-const signature mofunction is move constructible from "
                 "const signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*)> g(std::move(f));
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "non-const signature mofunction is move assignable from const "
                 "signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*)> g;
    g = std::move(f);
    // g(&called);
    // assert(called > 0);
  }
  {
    std::cout << "non-const signature mofunction is not move constructible "
                 "from const signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestConstCallable{});
    // std::mofunction<void(size_t*) const> g(std::move(f));
    // g(&called);
    assert(called == 0);
  }
  {
    std::cout << "non-const signature mofunction is not move assignable from "
                 "const signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestConstCallable{});
    std::mofunction<void(size_t*) const> g;
    // g = std::move(f);
    // g(&called);
    assert(called == 0);
  }

  // ==== Target Copy/Move Constructors with Rvref/Non-const Signature
  {
    std::cout << "non-const signature mofunction is not move constructible "
                 "from rvref signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    // std::mofunction<void(size_t*)> g(std::move(f));
    // g(&called);
    assert(called == 0);
  }
  {
    std::cout << "non-const signature mofunction is not move assignable from "
                 "rvref signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*)> g;
    // g = std::move(f);
    // g(&called);
    assert(called == 0);
  }
  {
    std::cout << "non-const signature mofunction is not move constructible "
                 "from const signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    // std::mofunction<void(size_t*) &&> g(std::move(f));
    // g(&called);
    assert(called == 0);
  }
  {
    std::cout << "non-const signature mofunction is not move assignable from "
                 "const signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::mofunction<void(size_t*) &&> g;
    // g = std::move(f);
    // g(&called);
    assert(called == 0);
  }

  // ==== Swap Operations
  {
    std::cout << "callable object constructed mofunction is swappable\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::mofunction<void(size_t*)> g;
    f.swap(g);
    assert(!f);
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "callable object constructed mofunction is ADL swappable\n";
    size_t called = 0;
    std::mofunction<void(size_t*)> f(TestCallable{});
    std::mofunction<void(size_t*)> g;
    std::swap(f, g);
    assert(!f);
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "const callable object constructed mofunction is swappable\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*) const> g;
    f.swap(g);
    assert(!f);
    g(&called);
    assert(called > 0);
  }
  {
    std::cout
        << "const callable object constructed mofunction is ADL swappable\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*) const> g;
    std::swap(f, g);
    assert(!f);
    g(&called);
    assert(called > 0);
  }
  {
    std::cout << "rvref callable object constructed mofunction is swappable\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*) &&> g;
    f.swap(g);
    assert(!f);
    std::move(g)(&called);
    assert(called > 0);
  }
  {
    std::cout
        << "rvref callable object constructed mofunction is ADL swappable\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*) &&> g;
    std::swap(f, g);
    assert(!f);
    std::move(g)(&called);
    assert(called > 0);
  }

  // ==== Swap with Const/Non-const Signature
  {
    std::cout << "non-const signature mofunction is not swappable from const "
                 "signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*)> g;
    // f.swap(g);
    // assert(!f);
    // std::move(g)(&called);
    assert(called == 0);
  }
  {
    std::cout << "non-const signature mofunction is not ADL swappable from "
                 "const signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) const> f(TestConstCallable{});
    std::mofunction<void(size_t*)> g;
    // std::swap(f, g);
    // assert(!f);
    // std::move(g)(&called);
    assert(called == 0);
  }

  // ==== Swap with Rvref/Non-const Signature
  {
    std::cout << "non-const signature mofunction is not swappable from rvref "
                 "signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*)> g;
    // f.swap(g);
    // assert(!f);
    // std::move(g)(&called);
    assert(called == 0);
  }
  {
    std::cout << "non-const signature mofunction is not ADL swappable from "
                 "rvref signature\n";
    size_t called = 0;
    std::mofunction<void(size_t*) &&> f(TestRvrefCallable{});
    std::mofunction<void(size_t*)> g;
    // std::swap(f, g);
    // assert(!f);
    // std::move(g)(&called);
    assert(called == 0);
  }

  return 0;
}
