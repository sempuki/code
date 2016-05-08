// Single Reader, Singler Writer Queue
// NOTE: a Bound of zero is "unbounded"

#include <cassert>
#include <iostream>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

template <typename Mutex>
auto take_unique_lock(Mutex &mutex) {
  return std::unique_lock<Mutex>{mutex};
}

template <typename Type, size_t Bound = 0>
class readwrite_locked_queue {
 public:
  void push(Type item) {
    auto writelock = take_unique_lock(writemutex_);
    
    while (is_write_full()) {
      writable_.wait(writelock);
    }

    assert(!is_write_full() && "Failed push condition");
    writequeue_.emplace_back(std::move(item));
    readable_.notify_one();
  }

  // contents must appear atomically
  void push(const std::vector<Type> &container) {
    assert(Bound == 0 || container.size() <= Bound);
    auto writelock = take_unique_lock(writemutex_);

    while (has_write_capactiy(container.size())) {
      writable_.wait(writelock);  // TODO: starvation prone
    }

    assert(!has_write_capactiy(container.size()) && "Failed push condition");
    writequeue_.insert(end(writequeue_), begin(container), end(container));
    readable_.notify_one();
  }

  Type pop() {
    auto readlock = take_unique_lock(readmutex_);

    if (is_read_empty()) {
      auto writelock = take_unique_lock(writemutex_);
      while (is_read_empty()) {
        std::swap(readqueue_, writequeue_);
        if (is_read_empty()) {
          readable_.wait(writelock);
        } else {
          writable_.notify_all();
        }
      }
    }

    assert(!is_read_empty() && "Failed pop condition");
    Type item{std::move(readqueue_.front())};
    readqueue_.pop_front();

    return item;
  }

 private:
  bool has_write_capactiy(size_t n) const {
    return Bound == 0 || writequeue_.size() + n <= Bound;
  }

  bool is_write_full() const {
    return Bound > 0 && writequeue_.size() >= Bound;
  }

  bool is_read_empty() const {
    return readqueue_.empty();
  }

  mutable std::mutex writemutex_;
  std::deque<Type> writequeue_;

  // TODO: std::hardware_destructive_interference_size
  std::condition_variable writable_;
  std::condition_variable readable_;

  mutable std::mutex readmutex_;
  std::deque<Type> readqueue_;
};

int main() {
  readwrite_locked_queue<int, 10> queue;
  std::cout << "pushing: " << std::endl;
  queue.push(5);
  std::cout << "popping: " << std::endl;
  std::cout << "found: " << queue.pop() << std::endl;
}
