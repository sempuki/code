/////////////////////////////////////////////////////////////////////////
// This is a minimal example that shows the 4 main building-blocks needed to
// write concurrent/async coroutine code.
//
// 1. A coroutine type that lets users write their coroutine logic
//    and call and co_await other coroutines that they write.
//    This allows composition of async coroutine code.
//
//    This example defines a basic `task<T>` type that fulfills this purpose.
//
// 2. A mechanism for launching concurrent async operations and later
//    waiting for launched concurrent work to complete.
//
//    To be able to have multiple coroutines executing independently we need
//    some way to introduce concurrency. And to ensure that we are able to
//    safely compose concurrent operations and shut down cleanly, we need
//    some way to be able to wait for concurrent operations to complete so
//    that we can. e.g. ensure that the completion of those concurrent operations
//    "happens before" the destruction of resources used by those concurrent
//    operations.
//
//    This example defines a simple `async_scope` facility that lets you
//    spawn multiple coroutines that can run independently, keeping an atomic
//    reference count of the number of unfinished tasks that have been launched.
//    It also gives a way to asynchronously wait until all tasks have completed.
//
// 3. A mechanism for blocking a thread until some async operation completes.
//
//    The main() function is a synchronous function and so if we launch some
//    async code we need some way to be able to block until that code completes
//    so that we don't return from main() until all concurrent work has been
//    joined.
//
//    This example defines a sync_wait() function that takes an awaitable which
//    it co_awaits and then blocks the current thread until the co_await expression
//    completes, returning the result of the co_await expression.
//
// 4. A mechanism for multiplexing multiple coroutines onto a set of worker threads.
//
//    One of the main reasons for writing asynchronous code is to allow a thread
//    to do something else while waiting for some operation to complete. This
//    requires some way to schedule/multiplex multiple coroutines onto a smaller
//    number of threads, typically using a queue and having an event-loop run by
//    each thread that allows it to do some work until that work suspends and then
//    pick up the next item in the queue and execute that in the meantime to keep
//    the thread busy.
//
//    This example provides a basic `manual_event_loop` implementation that allows
//    a coroutine to `co_await loop.schedule()` to suspend and enqueue itself to
//    the loop's queue, whereby a thread that is calling `loop.run()` will eventually
//    pick up that item and resume it.
//
//    In practice, such multiplexers often also support other kinds of scheduling
//    such as 'schedule when an I/O operation completes' or 'schedule when a time
//    elapses'.
//
// These 4 components are essential to being able to write asynchronous coroutine code.
//
// Different coroutine library implementations may structure these facilities in
// different ways, sometimes combining these items into one abstraction. e.g. sometimes
// a multiplexer implementation might combine items 2, 3 and 4 by providing a mechanism
// to launch a coroutine on that multiplexer and also wait for all launched work on
// that multiplexer.
//
// This example choses to separate them so that you can understand each component
// separately - each of the classes are relatively short (roughly 100 lines) so
// should hopefully be relatively easy to study.
//
// However, keeping them separate also generally gives better flexibility with
// how to compose them into an application. e.g. see how we can reuse async_scope
// in the nested_scopes() example below.
//
// This example also defines a number of helper concepts/traits needed by some of
// the implementations:
// - `awaitable` concept
// - `awaiter` concept
// - `await_result_t<T>` type-trait
// - `awaiter_type_t<T>` type-trait
// - `get_awaiter(x) -> awaiter` helper function

// And some other helpers:
// - `lazy_task` - useful for improving coroutine allocation-elision
// - `scope_guard`
//
//
// Please feel free to use this code however you like - it is primarily intended
// for learning coroutine mechanisms rather than necessarily as production-quality
// code. However, attribution is appreciated if you do use it somewhere.
//
// By Lewis Baker <lewissbaker@gmail.com>
/////////////////////////////////////////////////////////////////////////

#include <atomic>
#include <cassert>
#include <concepts>
#include <condition_variable>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <mutex>
#include <optional>
#include <semaphore>
#include <stop_token>
#include <thread>
#include <utility>
#include <variant>

///////////////////////////////////////////////////
// general helpers

#define FORCE_INLINE __attribute__((always_inline))

template <typename T>
concept decay_copyable = std::constructible_from<std::decay_t<T>, T>;

template <typename F>
struct scope_guard {
  F func;
  bool cancelled{false};

  template <typename F2>
    requires std::constructible_from<F, F2>
  explicit scope_guard(F2&& f) noexcept(std::is_nothrow_constructible_v<F, F2>)
    : func(static_cast<F2>(f)) {}

  scope_guard(scope_guard&& g) noexcept
    requires std::is_nothrow_move_constructible_v<F>
    : func(std::move(g.func)), cancelled(std::exchange(g.cancelled, true)) {}

  ~scope_guard() { call_now(); }

  void cancel() noexcept { cancelled = true; }

  void call_now() noexcept {
    if (!cancelled) {
      cancelled = true;
      func();
    }
  }
};

template <typename F>
scope_guard(F) -> scope_guard<F>;

///////////////////////////////////////////////////
// coroutine helpers

// Concept that checks if a type is a valid "awaiter" type.
// i.e. has the await_ready/await_suspend/await_resume methods.
//
// Note that we can't check whether await_suspend() is valid here because
// we don't know what type of coroutine_handle<> to test it with.
// So we just check for await_ready/await_resume and assume if it has both
// of those then it will also have the await_suspend() method.
template <typename T>
concept awaiter = requires(T& x) {
                    (x.await_ready() ? (void)0 : (void)0);
                    x.await_resume();
                  };

template <typename T>
concept _member_co_await = requires(T&& x) {
                             { static_cast<T&&>(x).operator co_await() } -> awaiter;
                           };

template <typename T>
concept _free_co_await = requires(T&& x) {
                           { operator co_await(static_cast<T&&>(x)) } -> awaiter;
                         };

template <typename T>
concept awaitable = _member_co_await<T> || _free_co_await<T> || awaiter<T>;

// get_awaiter(x) -> awaiter
//
// Helper function that tries to mimic what the compiler does in `co_await`
// expressions to obtain the awaiter for a given awaitable argument.
//
// It's not a perfect match, however, as we can't exactly match the overload
// resolution which combines both free-function overloads and member-function overloads
// of `operator co_await()` into a single overload-set.
//
// The `get_awaiter()` function will be an ambiguous call if a type has both
// a free-function `operator co_await()` and a member-function `operator co_await()`
// even if the compiler's overload resolution would not consider this to be
// ambiguous.
template <typename T>
  requires _member_co_await<T> decltype(auto)
get_awaiter(T&& x) noexcept(noexcept(static_cast<T&&>(x).operator co_await())) {
  return static_cast<T&&>(x).operator co_await();
}

template <typename T>
  requires _free_co_await<T> decltype(auto)
get_awaiter(T&& x) noexcept(operator co_await(static_cast<T&&>(x))) {
  return operator co_await(static_cast<T&&>(x));
}

template <typename T>
  requires awaiter<T> && (!_free_co_await<T> && !_member_co_await<T>)
T&& get_awaiter(T&& x) noexcept {
  return static_cast<T&&>(x);
}

template <typename T>
  requires awaitable<T>
using awaiter_type_t = decltype(get_awaiter(std::declval<T>()));

template <typename T>
  requires awaitable<T>
using await_result_t = decltype(std::declval<awaiter_type_t<T>&>().await_resume());

///////////////////////////////////////////////////
// task<T> - basic async task type

template <typename T>
struct task;

template <typename T>
struct task_promise {
  task<T> get_return_object() noexcept;

  std::suspend_always initial_suspend() noexcept { return {}; }

  struct final_awaiter {
    bool await_ready() noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<task_promise> h) noexcept {
      return h.promise().continuation;
    }
    [[noreturn]] void await_resume() noexcept { std::terminate(); }
  };

  final_awaiter final_suspend() noexcept { return {}; }

  template <typename U>
    requires std::convertible_to<U, T>
  void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>) {
    result.template emplace<1>(std::forward<U>(value));
  }

  void unhandled_exception() noexcept { result.template emplace<2>(std::current_exception()); }

  std::coroutine_handle<> continuation;
  std::variant<std::monostate, T, std::exception_ptr> result;
};

template <>
struct task_promise<void> {
  task<void> get_return_object() noexcept;

  std::suspend_always initial_suspend() noexcept { return {}; }

  struct final_awaiter {
    bool await_ready() noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<task_promise> h) noexcept {
      return h.promise().continuation;
    }
    [[noreturn]] void await_resume() noexcept { std::terminate(); }
  };

  final_awaiter final_suspend() noexcept { return {}; }

  void return_void() noexcept { result.emplace<1>(); }

  void unhandled_exception() noexcept { result.emplace<2>(std::current_exception()); }

  struct empty {};

  std::coroutine_handle<> continuation;
  std::variant<std::monostate, empty, std::exception_ptr> result;
};

template <typename T>
struct [[nodiscard]] task {
 private:
  using handle_t = std::coroutine_handle<task_promise<T>>;
  handle_t coro;

  struct awaiter {
    handle_t coro;
    bool await_ready() noexcept { return false; }

    handle_t await_suspend(std::coroutine_handle<> h) noexcept {
      coro.promise().continuation = h;
      return coro;
    }

    T await_resume() {
      if (coro.promise().result.index() == 2) {
        std::rethrow_exception(std::get<2>(std::move(coro.promise().result)));
      }

      assert(coro.promise().result.index() == 1);

      if constexpr (!std::is_void_v<T>) {
        return std::get<1>(std::move(coro.promise().result));
      }
    }
  };

  friend class task_promise<T>;

  explicit task(handle_t h) noexcept : coro(h) {}

 public:
  using promise_type = task_promise<T>;

  task(task&& other) noexcept : coro(std::exchange(other.coro, {})) {}

  ~task() {
    if (coro) coro.destroy();
  }

  awaiter operator co_await() && { return awaiter{coro}; }
};

template <typename T>
task<T> task_promise<T>::get_return_object() noexcept {
  return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
}

task<void> task_promise<void>::get_return_object() noexcept {
  return task<void>{std::coroutine_handle<task_promise<void>>::from_promise(*this)};
}

////////////////////////////////////////
// async_scope
//
// Used to launch new tasks and then later wait until all tasks have completed.

struct async_scope {
 private:
  struct detached_task {
    struct promise_type {
      async_scope& scope;

      promise_type(async_scope& scope, auto&) noexcept : scope(scope) {}

      detached_task get_return_object() noexcept { return {}; }

      std::suspend_never initial_suspend() noexcept {
        scope.add_ref();
        return {};
      }

      struct final_awaiter {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          async_scope& s = h.promise().scope;
          h.destroy();
          s.notify_task_finished();
        }
        void await_resume() noexcept {}
      };

      final_awaiter final_suspend() noexcept { return {}; }

      void return_void() noexcept {}

      [[noreturn]] void unhandled_exception() noexcept { std::terminate(); }
    };
  };

  template <typename A>
  detached_task spawn_detached_impl(A a) {
    co_await std::forward<A>(a);
  }

  void add_ref() noexcept { ref_count.fetch_add(ref_increment, std::memory_order_relaxed); }

  void notify_task_finished() noexcept {
    std::size_t oldValue = ref_count.load(std::memory_order_acquire);
    assert(oldValue >= ref_increment);

    if (oldValue > (joiner_flag + ref_increment)) {
      oldValue = ref_count.fetch_sub(ref_increment, std::memory_order_acq_rel);
    }

    if (oldValue == (joiner_flag + ref_increment)) {
      // last ref and there is a joining coroutine -> resume the coroutien
      joiner.resume();
    }
  }

  struct join_awaiter {
    async_scope& scope;

    bool await_ready() { return scope.ref_count.load(std::memory_order_acquire) == 0; }

    bool await_suspend(std::coroutine_handle<> h) noexcept {
      scope.joiner = h;
      std::size_t oldValue = scope.ref_count.fetch_add(joiner_flag, std::memory_order_acq_rel);
      return (oldValue != 0);
    }

    void await_resume() noexcept {}
  };

  static constexpr std::size_t joiner_flag = 1;
  static constexpr std::size_t ref_increment = 2;

  std::atomic<std::size_t> ref_count{0};
  std::coroutine_handle<> joiner;

 public:
  template <typename A>
    requires decay_copyable<A> && awaitable<std::decay_t<A>>
  void spawn_detached(A&& a) {
    spawn_detached_impl(std::forward<A>(a));
  }

  [[nodiscard]] join_awaiter join_async() noexcept { return join_awaiter{*this}; }
};

////////////////////////////////////////////////////////////////
// sync_wait()

template <typename Task>
await_result_t<Task> sync_wait(Task&& t) {
  struct _void {};
  using return_type = await_result_t<Task>;
  using storage_type =
    std::add_pointer_t<std::conditional_t<std::is_void_v<return_type>, _void, return_type>>;
  using result_type = std::variant<std::monostate, storage_type, std::exception_ptr>;

  struct _sync_task {
    struct promise_type {
      std::binary_semaphore sem{0};
      result_type result;

      _sync_task get_return_object() noexcept {
        return _sync_task{std::coroutine_handle<promise_type>::from_promise(*this)};
      }

      struct final_awaiter {
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
          // Now that coroutine has suspended we can signal the semaphore,
          // unblocking the waiting thread. The other thread will then
          // destroy this coroutine (which is safe because it is suspended).
          h.promise().sem.release();
        }
        void await_resume() noexcept {}
      };

      std::suspend_always initial_suspend() noexcept { return {}; }

      final_awaiter final_suspend() noexcept { return {}; }

      using non_void_return_type =
        std::conditional_t<std::is_void_v<return_type>, _void, return_type>;

      final_awaiter yield_value(non_void_return_type&& x)
        requires(!std::is_void_v<return_type>)
      {
        // Note that we just store the address here and then suspend
        // and unblock the waiting thread which then copies/moves the
        // result from this address directly to the return-value of
        // sync_wait(). This avoids having to make an extra intermediate
        // copy of the result value.
        result.template emplace<1>(std::addressof(x));
        return {};
      }

      void return_void() noexcept { result.template emplace<1>(); }

      void unhandled_exception() noexcept { result.template emplace<2>(std::current_exception()); }
    };

    using handle_t = std::coroutine_handle<promise_type>;
    handle_t coro;

    explicit _sync_task(handle_t h) noexcept : coro(h) {}
    _sync_task(_sync_task&& o) noexcept : coro(std::exchange(o.coro, {})) {}
    ~_sync_task() {
      if (coro) coro.destroy();
    }

    // The force-inline here is required to get the _sync_task coroutine elided.
    // Otherwise the compiler doesn't know that this function hasn't modified 'coro'
    // member variable and so can't deduce that it's always destroyed before sync_wait()
    // returns.
    FORCE_INLINE return_type run() {
      coro.resume();
      coro.promise().sem.acquire();

      auto& result = coro.promise().result;
      if (result.index() == 2) {
        std::rethrow_exception(std::get<2>(std::move(result)));
      }

      assert(result.index() == 1);

      if constexpr (!std::is_void_v<return_type>) {
        return static_cast<return_type&&>(*std::get<1>(result));
      }
    }
  };

  return [&]() -> _sync_task {
    if constexpr (std::is_void_v<return_type>) {
      co_await static_cast<Task&&>(t);
    } else {
      // use co_yield instead of co_return so we suspend while the
      // potentially temporary result of co_await is still alive.
      co_yield co_await static_cast<Task&&>(t);
    }
  }()
                    .run();
}

/////////////////////////////////////////////////
// manual_event_loop
//
// A simple scheduler context with an intrusive list.
//
// Uses mutex/condition_variable for synchronisation and supports
// multiple work threads running tasks.

struct manual_event_loop {
 private:
  struct queue_item {
    queue_item* next;
    std::coroutine_handle<> coro;
  };

  std::mutex mut;
  std::condition_variable cv;
  queue_item* head{nullptr};
  queue_item* tail{nullptr};

  void enqueue(queue_item* item) noexcept {
    std::lock_guard lock{mut};
    item->next = nullptr;
    if (head == nullptr) {
      head = item;
    } else {
      tail->next = item;
    }
    tail = item;
    cv.notify_one();
  }

  queue_item* pop_item() noexcept {
    queue_item* front = head;
    if (head != nullptr) {
      head = front->next;
      if (head == nullptr) {
        tail = nullptr;
      }
    }
    return front;
  }

  struct schedule_awaitable {
    manual_event_loop* loop;
    queue_item item;

    explicit schedule_awaitable(manual_event_loop& loop) noexcept : loop(&loop) {}

    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<> coro) noexcept {
      item.coro = coro;
      loop->enqueue(&item);
    }
    void await_resume() noexcept {}
  };

 public:
  schedule_awaitable schedule() noexcept { return schedule_awaitable{*this}; }

  void run(std::stop_token st) noexcept {
    std::stop_callback cb{st, [&]() noexcept {
                            std::lock_guard lock{mut};
                            cv.notify_all();
                          }};

    std::unique_lock lock{mut};
    while (true) {
      cv.wait(lock, [&]() noexcept { return head != nullptr || st.stop_requested(); });
      if (st.stop_requested()) {
        return;
      }

      queue_item* item = pop_item();

      lock.unlock();
      item->coro.resume();
      lock.lock();
    }
  }
};

////////////////////////////////////////////////
// Helper for improving allocation elision for composed operations.
//
// Instead of doing something like:
//
//   task<T> h(int arg);
//   scope.spawn_detached(h(42));
//
// which will generally separately allocate the h() coroutine as well
// as the internal detached_task coroutine, if we write:
//
//   scope.spawn_detached(lazy_task{[] { return h(42); }});
//
// then this defers calling the `h()` coroutine function to the evaluation
// of `operator co_await()` in the `detached_task` coroutine, which then
// permits the compiler to elide the allocation of `h()` coroutine and
// combine its storage into the `detached_task` coroutine state, meaning
// that we now have one allocation per spawned task instead of two.

template <typename F>
struct lazy_task {
  F func;
  using task_t = std::invoke_result_t<F&>;
  using awaiter_t = awaiter_type_t<task_t>;

  struct awaiter {
    task_t task;
    awaiter_t inner;

    explicit awaiter(F& func) noexcept(
      std::is_nothrow_invocable_v<F&>&& noexcept(get_awaiter(static_cast<task_t&&>(task))))
      : task(func()), inner(get_awaiter(static_cast<task_t&&>(task))) {}

    decltype(auto) await_ready() noexcept(noexcept(inner.await_ready())) {
      return inner.await_ready();
    }
    decltype(auto) await_suspend(auto h) noexcept(noexcept(inner.await_suspend(h))) {
      return inner.await_suspend(h);
    }
    decltype(auto) await_resume() noexcept(noexcept(inner.await_resume())) {
      return inner.await_resume();
    }
  };

  awaiter operator co_await() noexcept(std::is_nothrow_constructible_v<awaiter, F&>) {
    return awaiter{func};
  }
};

template <typename F>
lazy_task(F) -> lazy_task<F>;

/////////////////////////////////////////////////
// Example code

#include <unistd.h>

static task<int> f(int i) {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1ms);

  co_return i;
}

static task<int> g(int i, manual_event_loop& loop) {
  co_await loop.schedule();
  int x = co_await f(i);
  co_return x + 1;
}

static task<void> h(int i, manual_event_loop& loop) {
  int x = co_await g(i, loop);
  auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
  std::printf("[%u] %i -> %i (on %i)\n", (unsigned int)ts, i, x, (int)::gettid());
}

static task<void> nested_scopes(int x, manual_event_loop& loop) {
  co_await loop.schedule();

  async_scope scope;
  try {
    for (int i = 0; i < 10; ++i) {
      scope.spawn_detached(h(i, loop));
    }
  } catch (...) {
    std::printf("failure!\n");
  }

  co_await scope.join_async();

  std::printf("nested %i done\n", x);
  std::fflush(stdout);
}

int main() {
  manual_event_loop loop;

  std::jthread thd{[&](std::stop_token st) {
    loop.run(st);
  }};
  std::jthread thd2{[&](std::stop_token st) {
    loop.run(st);
  }};

  std::printf("starting example\n");

  {
    async_scope scope;
    scope_guard join_on_exit{[&] {
      sync_wait(scope.join_async());
    }};

    for (int i = 0; i < 10; ++i) {
      // Use lazy_task here so that h() coroutine allocation is elided
      // and incorporated into spawn_detached() allocation.
      scope.spawn_detached(lazy_task{[i, &loop] {
        return h(i, loop);
      }});
    }
  }

  std::printf("starting nested_scopes example\n");

  {
    async_scope scope;
    scope_guard join_on_exit{[&] {
      sync_wait(scope.join_async());
    }};

    for (int i = 0; i < 10; ++i) {
      scope.spawn_detached(lazy_task{[i, &loop] {
        return nested_scopes(i, loop);
      }});
    }
  }

  return 0;
}

