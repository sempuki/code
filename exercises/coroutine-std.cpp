// Copyright 2023 Ryan McDougall

#include <cassert>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>

struct suspend_never {
  bool await_ready() const noexcept {
    std::cout << "await ready: suspend never\n";
    return true;
  }
  void await_suspend(std::coroutine_handle<>) const noexcept {
    std::cout << "await suspend: suspend never\n";
  }
  void await_resume() const noexcept { std::cout << "await resume: suspend never\n"; }
};

struct suspend_always {
  bool await_ready() const noexcept {
    std::cout << "await ready: suspend always\n";
    return true;
  }
  void await_suspend(std::coroutine_handle<>) const noexcept {
    std::cout << "await suspend: suspend always\n";
  }
  void await_resume() const noexcept { std::cout << "await resume: suspend always\n"; }
};

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

    suspend_never initial_suspend() {
      std::cout << "promise initial suspend\n";
      return {};
    }
    suspend_never final_suspend() noexcept {
      std::cout << "promise final suspend\n";
      return {};
    }
    void unhandled_exception() { std::cout << "unhandled exception\n"; }
    future_type get_return_object() {
      std::cout << "promise get return object\n";
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

future_type f(int v) {
  std::cout << "start f with " << v << "\n";
  awaitable_type a;
  std::cout << "await a\n";
  (co_await a).foo();
  std::cout << "end f\n";
  co_return future_type{};
}

int main(int argc, char** argv) {
  std::cout << "using std coroutines\n";
  std::cout << "start main\n";
  auto future = f(5);
  std::cout << "use future\n";
  future.doit();
  std::cout << "end main\n";
}

