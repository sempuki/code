#ifndef SRC_ASYNC_LATCH_HPP_
#define SRC_ASYNC_LATCH_HPP_

#include <atomic>

namespace async {

class latch {
 public:
  explicit latch(size_t limit = 1) :
    limit_ {limit} {}

 public:
  bool open() const { return count_ >= limit_; }
  size_t count() const { return count_; }
  size_t limit() const { return limit_; }

 public:
  void await() const { while (count_ < limit_) {} }
  size_t ready() { return count_++; }
  void reset() { count_ = 0; }

 private:
  std::atomic<size_t> count_ {0};
  const size_t limit_;
};

}  // namespace async

#endif
