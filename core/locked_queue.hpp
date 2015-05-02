#ifndef SRC_ASYNC_LOCKEDQUEUE_HPP_
#define SRC_ASYNC_LOCKEDQUEUE_HPP_

#include <mutex>
#include <condition_variable>

#include "core/common.hpp"

namespace async {

// NOTE: a Bound of zero is "unbounded"
template <typename Type, size_t Bound = 0>
class locked_queue {
 public:
  bool empty() const {
    std::unique_lock<std::mutex> lock {mutex_};
    return queue_.empty();
  }

  size_t size() const {
    std::unique_lock<std::mutex> lock {mutex_};
    return queue_.size();
  }

 public:
  void push(Type item) {
    std::unique_lock<std::mutex> lock {mutex_};

    if (is_bound()) {
      full_.wait(lock, [this]{ return !is_full(); });
    }

    queue_.push(std::move(item));
    empty_.notify_one();
  }

  Type pop() {
    std::unique_lock<std::mutex> lock {mutex_};

    empty_.wait(lock, [this]{ return !is_empty(); });

    auto item = take_front();
    if (is_bound()) {
      full_.notify_one();
    }
    return item;
  }

 public:
  bool try_push(Type item) {
    std::unique_lock<std::mutex> lock {mutex_};

    auto proceed = !is_full();
    if (proceed) {
      queue_.push(std::move(item));
      empty_.notify_one();
    }
    return proceed;
  }

  bool try_pop(Type &item) {  // NOLINT
    std::unique_lock<std::mutex> lock {mutex_};

    auto proceed = !is_empty();
    if (proceed) {
      item = take_front();
      if (is_bound()) {
        full_.notify_one();
      }
    }
    return proceed;
  }

 private:
  Type take_front() {
    auto item = std::move(queue_.front()); queue_.pop();
    return item;
  }

  bool is_bound() const {
    return Bound > 0;
  }

  bool is_full() const {
    return Bound > 0 && queue_.size() >= Bound;
  }

  bool is_empty() const {
    return queue_.empty();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable full_;
  std::condition_variable empty_;
  std::queue<Type> queue_;
};

}  // namespace async

#endif
