// Copyright 2023 Ryan McDougall
// See:
//  https://lewissbaker.github.io/2017/09/25/coroutine-theory
//  https://lewissbaker.github.io/2017/11/17/understanding-operator-co-await
//  https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type
//  https://devblogs.microsoft.com/oldnewthing/20191209-00/?p=103195
//  https://devblogs.microsoft.com/oldnewthing/20191216-00/?p=103217
//  https://devblogs.microsoft.com/oldnewthing/20210331-00/?p=105028

#include <cassert>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <map>
#include <tuple>
#include <type_traits>

struct __stack {
  void destroy(){
    //...
  };
};

struct __state_base {
  void* suspension_point = nullptr;
  __stack local_variables;
  virtual ~__state_base() = default;
};

template <typename Promise>
struct __promised_state_base : public __state_base {
  Promise promise;
};

template <typename ReturnFuture, typename... Args>
struct __state : public __promised_state_base<
                   typename std::coroutine_traits<ReturnFuture, Args...>::promise_type> {
  std::tuple<Args...> arguments;
};

std::map<std::uintptr_t, __state_base*> __state_registry;

template <typename Promise = void>
struct coroutine_handle;

template <>
struct coroutine_handle<void> {
  __state_base* state;
};

// For GCC see __builtin_coro_{promise, done, resume, destroy}
template <typename Promise>
struct coroutine_handle {
  __promised_state_base<Promise>* state = nullptr;
  bool done = false;

  operator coroutine_handle<>() const { return coroutine_handle<>{state}; }
  operator bool() const { return state; }

  void resume() const {}
  void destroy() const { delete state; }

  void* address() const { return state; }
  static coroutine_handle from_address(void* addr) { return coroutine_handle{addr}; }

  Promise& promise() { return state->promise; }
  static coroutine_handle from_promise(Promise& p) {
    return coroutine_handle{dynamic_cast<__promised_state_base<Promise>>(
      __state_registry[static_cast<std::uintptr_t>(&p)])};
  }
};

template <class Promise>
struct std::hash<coroutine_handle<Promise>> {
  std::size_t operator()(const coroutine_handle<Promise>& h) const {
    return static_cast<std::uintptr_t>(h.state);
  }
};

template <typename ReturnFuture, typename... Args>
void __do_coroutine(void* body, Args&&... args) {
  auto* state = new __state<ReturnFuture, Args...>{body, std::forward<Args>(args)...};
  auto& arguments = state->arguments;
  auto& promise = state->promise;

  // See coroutine_handle::from_promise.
  __state_registry[static_cast<std::uintptr_t>(&promise)] = state;

  using Promise = decltype(state->promise);
  if constexpr (std::is_constructible_v<Promise, Args...>) {
    promise = Promise(arguments);
  } else {
    promise = Promise();
  }

  auto return_object = promise.get_return_object();

  try {
    // Lazy coroutines aways suspend, eager never suspend.
    __co_await(state, promise.initial_suspend(), return_object);
    std::coroutine_handle<Promise>::from_promise(promise).resume();
  } catch (...) {
    promise.unhandled_exception();
  }

  __co_await(state, promise.final_suspend(), return_object);
  delete state;
}

template <typename T>
constexpr bool has_any_await_transform_member_v = false;
template <typename T>
constexpr bool has_member_operator_co_await_v = false;
template <typename T>
constexpr bool has_non_member_operator_co_await_v = false;

template <typename Promise, typename Awaitable>
decltype(auto) get_awaitable(Promise& promise, Awaitable&& awaitable) {
  if constexpr (has_any_await_transform_member_v<Promise>)
    return promise.await_transform(static_cast<Awaitable&&>(awaitable));
  else
    return static_cast<Awaitable&&>(awaitable);
}

template <typename Awaitable>
decltype(auto) get_awaiter(Awaitable&& awaitable) {
  if constexpr (has_member_operator_co_await_v<Awaitable>)
    return static_cast<Awaitable&&>(awaitable).operator co_await();
  else if constexpr (has_non_member_operator_co_await_v<Awaitable&&>)
    return operator co_await(static_cast<Awaitable&&>(awaitable));
  else
    return static_cast<Awaitable&&>(awaitable);
}

// Calling .resume() on a coroutine_handle reactivates a suspended coroutine at the
// <resume-point>. The call to .resume() will return when the coroutine next hits a
// <return-to-caller-or-resumer> point.

template <typename Promise, typename Expression, typename ReturnFuture>
decltype(auto) __co_await(__promised_state_base<Promise>* state,
                          Expression&& expr,
                          ReturnFuture& return_future) {
  auto& promise = state->promise;
  auto&& awaitable = get_awaitable(promise, static_cast<Expression>(expr));
  auto&& awaiter = get_awaiter(static_cast<decltype(awaitable)>(awaitable));

  if (!awaiter.await_ready()) {
    using await_suspend_result_type =
      decltype(awaiter.await_suspend(std::coroutine_handle<Promise>::from_promise(promise)));

    // <suspend-coroutine>

    if constexpr (std::is_void_v<await_suspend_result_type>) {
      awaiter.await_suspend(std::coroutine_handle<Promise>::from_promise(promise));
      // <return-to-caller-or-resumer>(return_future)
    } else {
      static_assert(std::is_same_v<await_suspend_result_type, bool>,
                    "await_suspend() must return 'void' or 'bool'.");

      if (awaiter.await_suspend(std::coroutine_handle<Promise>::from_promise(promise))) {
        // <return-to-caller-or-resumer>(return_future)
      }
    }

    // <resume-point>
  }

  return awaiter.await_resume();
}

template <typename Promise, typename... ReturnValues>
void __co_return(__promised_state_base<Promise>* state, ReturnValues&&... values) {
  auto& promise = state->promise;
  if (sizeof...(ReturnValues)) {
    promise.return_value(std::forward<ReturnValues>(values)...);
  } else {
    promise.return_void();
  }

  state->local_variables.destroy();

  __co_await(state, promise.final_suspend());
}

// future/promise is protocol for passing values between coroutine and caller
// and it set up at init time within the coroutine state and valid until completion
// future -> promise is via `my_future::promise`
// promise -> future is via `my_promise::get_return_object`
// get return object is usually used to to pass coroutine_handle to future
// my_lib::do_something is a coroutine factory that accepts an int and generates a coroutine, and
// then returns a coro_widget with a handle to that coroutine.

// Awaiter is a protocol to inject logic into how suspension actually works.co_await works on
// awaiters. You offer an awaitable and the compiler tries to make an awaiter. Most awaitables are
// simply their own awaiters.

// co_yield/_return are "suspend back" for offering outputs.
// co_await are "suspend forward" for requiring inputs.

// Goal: download N files async in parallel where the code reads sync/sequential.
// Should co_await on a common init task

#include <coroutine>
#include <iostream>

struct X {
  X() { std::cout << "X default constructed\n"; }
  X(const X&) { std::cout << "X copy constructed\n"; }
  X(X&&) { std::cout << "X move constructed\n"; }
  X& operator=(const X&) {
    std::cout << "X copy assigned\n";
    return *this;
  }
  X& operator=(X&&) {
    std::cout << "X move assigned\n";
    return *this;
  }
  ~X() { std::cout << "X destroyed\n"; }
  void foo() { std::cout << "X foo'd\n"; }
};

struct future_type {
  future_type() { std::cout << "future default constructed\n"; }
  future_type(const future_type&) { std::cout << "future copy constructed\n"; }
  future_type(future_type&&) { std::cout << "future move constructed\n"; }
  future_type& operator=(const future_type&) {
    std::cout << "future copy assigned\n";
    return *this;
  }
  future_type& operator=(future_type&&) {
    std::cout << "future move assigned\n";
    return *this;
  }
  ~future_type() { std::cout << "future destroyed\n"; }

  struct promise_type {
    promise_type() { std::cout << "promise default constructed\n"; }
    promise_type(const promise_type&) { std::cout << "promise copy constructed\n"; }
    promise_type(promise_type&&) { std::cout << "promise move constructed\n"; }
    promise_type& operator=(const promise_type&) {
      std::cout << "promise copy assigned\n";
      return *this;
    }
    promise_type& operator=(promise_type&&) {
      std::cout << "promise move assigned\n";
      return *this;
    }
    ~promise_type() { std::cout << "promise destroyed\n"; }

    std::suspend_never initial_suspend() {
      std::cout << "initial suspend\n";
      return {};
    }
    std::suspend_never final_suspend() noexcept {
      std::cout << "final suspend\n";
      return {};
    }
    void unhandled_exception() { std::cout << "unhandled exception\n"; }
    future_type get_return_object() {
      std::cout << "get return object\n";
      return {this};
    }
    void return_value(future_type v) {}
  };

  future_type(promise_type* p) : promise{p} { std::cout << "future promise-constructed\n"; }
  promise_type* promise = nullptr;

  void doit() { std::cout << "doit!\n"; }
};

struct awaitable_type {
  awaitable_type() { std::cout << "awaitable default constructed\n"; }
  awaitable_type(const awaitable_type&) { std::cout << "awaitable copy constructed\n"; }
  awaitable_type(awaitable_type&&) { std::cout << "awaitable move constructed\n"; }
  awaitable_type& operator=(const awaitable_type&) {
    std::cout << "awaitable copy assigned\n";
    return *this;
  }
  awaitable_type& operator=(awaitable_type&&) {
    std::cout << "awaitable move assigned\n";
    return *this;
  }
  ~awaitable_type() { std::cout << "awaitable destroyed\n"; }

  bool await_ready() {
    std::cout << "await ready\n";
    return false;
  }
  bool await_suspend(auto handle) {
    std::cout << "await suspend\n";
    return false;
  }
  X await_resume() {
    std::cout << "await resume\n";
    return {};
  }
  auto operator co_await() {
    std::cout << "operator await\n";
    return *this;
  }
};

future_type f() {
  std::cout << "start f\n";
  awaitable_type a;
  std::cout << "await a\n";
  (co_await a).foo();
  std::cout << "end f\n";
  // co_return future_type{};
}

int main() {
  std::cout << "start main\n";
  auto future = f();
  std::cout << "use future\n";
  future.doit();
  std::cout << "end main\n";
}

