#ifndef SRC_ASYNC_SPINMUTEX_HPP_
#define SRC_ASYNC_SPINMUTEX_HPP_

#include <atomic>

namespace async {

class spin_mutex {
 public:
  void lock() { while (locked_.exchange(true)) {} }
  void unlock() { locked_ = false; }

 private:
  std::atomic<bool> locked_ {false};
};

}  // namespace async

#endif
