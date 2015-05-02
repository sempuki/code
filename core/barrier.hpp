#ifndef SRC_ASYNC_BARRIER_HPP_
#define SRC_ASYNC_BARRIER_HPP_

#include <atomic>

namespace async {

class barrier {
 public:
  explicit barrier(size_t limit) :
    limit_ {limit} {}

 public:
  bool open() const { return count_ >= limit_; }
  size_t count() const { return count_; }
  size_t limit() const { return limit_; }

 public:
  void await() { count_++, while (count_ < limit_) {} }
  void reset() { count_ = 0; }

 private:
  std::atomic<size_t> count_ {0};
  const size_t limit_;
};

}  // namespace async

#endif
