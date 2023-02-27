// Copyright 2023 Ryan McDougall

#include <coroutine>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

#include "simplelog.hpp"
#include "simpletest.hpp"

constexpr bool simplelog_on = true;

struct suspend_never {
  bool await_ready() const noexcept { HERE return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept { HERE; }
  void await_resume() const noexcept { HERE; }
};

struct suspend_always {
  bool await_ready() const noexcept { HERE return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept { HERE; }
  void await_resume() const noexcept { HERE; }
};

struct future_type {
  future_type() { HERE; }
  future_type(const future_type&) { HERE; }
  future_type(future_type&&) { HERE; }
  future_type& operator=(const future_type&) { HERE return *this; }
  future_type& operator=(future_type&&) { HERE return *this; }
  ~future_type() { HERE; }

  struct promise_type {
    promise_type() { HERE; }
    promise_type(const promise_type&) { HERE; }
    promise_type(promise_type&&) { HERE; }
    promise_type& operator=(const promise_type&) { HERE return *this; }
    promise_type& operator=(promise_type&&) { HERE return *this; }
    ~promise_type() { HERE; }

    suspend_never initial_suspend() { HERE return {}; }
    suspend_never final_suspend() noexcept { HERE return {}; }
    void unhandled_exception() { HERE; }
    future_type get_return_object() { HERE return {this}; }
    void return_value(future_type v) {}
  };

  future_type(promise_type* p) : promise{p} { HERE; }
  promise_type* promise = nullptr;

  void doit() { HERE; }
};

struct X {
  X() { HERE; }
  X(const X&) { HERE; }
  X(X&&) { HERE; }
  X& operator=(const X&) { HERE return *this; }
  X& operator=(X&&) { HERE return *this; }
  ~X() { HERE; }
  void foo() { HERE; }
};

struct awaitable_type {
  awaitable_type() { HERE; }
  awaitable_type(const awaitable_type&) { HERE; }
  awaitable_type(awaitable_type&&) { HERE; }
  awaitable_type& operator=(const awaitable_type&) { HERE return *this; }
  awaitable_type& operator=(awaitable_type&&) { HERE return *this; }
  ~awaitable_type() { HERE; }

  bool await_ready() { HERE return false; }
  bool await_suspend(auto handle) { HERE return false; }
  X await_resume() { HERE return {}; }
  auto operator co_await() { HERE return *this; }
};

future_type f(int v) {
  std::cout << "start f with " << v << "\n";
  awaitable_type a;
  std::cout << "await a\n";
  (co_await a).foo();
  std::cout << "end f\n";
  co_return future_type{};
}

int main(int argc, char** argv) {
  {
    using some = suspend_never;
    const some a;

    IT_SHOULD(return_true_from_await_ready, {  //
      EXPECT(a.await_ready() == true);
    });

    IT_CAN(call_await_suspend_with_coroutine_handle, {  //
      a.await_suspend(std::coroutine_handle<>{});
    });

    IT_CAN(call_await_resume, {  //
      a.await_resume();
    });
  }
  {
    using some = suspend_always;
    const some a;

    IT_SHOULD(return_true_from_await_ready, {  //
      EXPECT(a.await_ready() == false);
    });

    IT_CAN(call_await_suspend_with_coroutine_handle, {  //
      a.await_suspend(std::coroutine_handle<>{});
    });

    IT_CAN(call_await_resume, {  //
      a.await_resume();
    });
  }
  {
    struct a_promise_type;

    struct a_future_type {
      using promise_type = a_promise_type;
      a_future_type(a_promise_type* p) : promise{p} {}
      a_promise_type* promise = nullptr;
    };

    struct a_promise_type {
      suspend_never initial_suspend() { return {}; }
      suspend_never final_suspend() noexcept { return {}; }
      void unhandled_exception() {}
      a_future_type get_return_object() { return {this}; }
      void return_value(a_future_type v) {}
    };

    IT_CAN(declare_a_nested_promise, {  //
      a_future_type::promise_type p;
    });

    using some = a_future_type::promise_type;
    some promise;

    IT_CAN(create_a_future_from_get_return_object, {  //
      a_future_type future = promise.get_return_object();
    });

    IT_SHOULD(link_the_future_and_promise_together, {  //
      a_future_type future = promise.get_return_object();
      EXPECT(future.promise == &promise);
    });

    IT_CAN(await_the_result_of_initial_suspend, {  //
      auto a = promise.initial_suspend();
      bool b = a.await_ready();
      a.await_suspend(std::coroutine_handle<>{});
      a.await_resume();
    });

    IT_CAN(await_the_result_of_final_suspend, {  //
      auto a = promise.final_suspend();
      bool b = a.await_ready();
      a.await_suspend(std::coroutine_handle<>{});
      a.await_resume();
    });
  }

  std::cout << "using std coroutines\n";
  std::cout << "start main\n";
  auto future = f(5);
  std::cout << "use future\n";
  future.doit();
  std::cout << "end main\n";
}

