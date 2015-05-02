#ifndef SRC_ASYNC_SIMPLELOCKEDQUEUE_HPP_
#define SRC_ASYNC_SIMPLELOCKEDQUEUE_HPP_

#include <mutex>
#include <numeric>

#include "core/algorithms.hpp"
#include "core/Counters.hpp"

namespace async {

namespace policy {

struct Empty {
  void on_push() {}
  void on_pop() {}
};

struct Counted {
  core::Counter &counter;
  Counted(core::Counter &counter) :
    counter {counter} {}

  void on_push() { ++counter; }
  void on_pop() { --counter; }
};

}  // namespace policy


template <typename Type, typename QueuePolicy = policy::Empty, typename Container = std::deque<Type>>
class simple_locked_queue : private QueuePolicy {
 public:
  simple_locked_queue() = default;
  simple_locked_queue(QueuePolicy policy) :
    QueuePolicy {policy} {}

 public:
  void push(Type &&item) {
    std::lock_guard<std::mutex> lock {mutex_};
    container_.push_back(std::move(item));
    QueuePolicy::on_push();
  }

  void push(const Type &item) {
    std::lock_guard<std::mutex> lock {mutex_};
    container_.push_back(item);
    QueuePolicy::on_push();
  }

 public:
  bool try_pop(Type &item) {  // NOLINT
    return try_pop(item, core::always);
  }

  template <typename Predicate>
  bool try_pop(Type &item, Predicate pred) {  // NOLINT
    std::lock_guard<std::mutex> lock {mutex_};
    auto success = !container_.empty() && pred(container_.front());
    if (success) {
      item = std::move(container_.front());
      container_.pop_front();
      QueuePolicy::on_pop();
    }
    return success;
  }

 public:
  template <typename Predicate>
  size_t filter(Predicate pred) {
    std::lock_guard<std::mutex> lock {mutex_};
    return core::remove_erase_if(container_, pred);
  }

  template <typename Accumulation, typename Operation>
  size_t accumulate(Accumulation init, Operation op) {
    std::lock_guard<std::mutex> lock {mutex_};
    return std::accumulate(begin(container_), end(container_), init, op);
  }

 public:
  void clear() {
    std::lock_guard<std::mutex> lock {mutex_};
    container_.clear();
  }

  size_t size() const {
    std::lock_guard<std::mutex> lock {mutex_};
    return container_.size();
  }

 private:
  mutable std::mutex mutex_;
  Container container_;
};

}  // namespace async

#endif
