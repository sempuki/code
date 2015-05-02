#ifndef SRC_CORE_CONTAINERS_HPP_
#define SRC_CORE_CONTAINERS_HPP_

namespace core {

template <typename Type>
using uninitialized = typename std::aligned_storage<sizeof(Type), alignof(Type)>::type;

template <typename Key, typename Value>
using flat_map = std::vector<std::pair<Key, Value>>;

template <typename Type, typename Compare = std::less<Type>>
class stable_priority_queue {
 public:
  using value_type = std::pair<Type, uint64_t>;
  using const_iterator = typename std::vector<value_type>::const_iterator;
  using iterator = typename std::vector<value_type>::iterator;

 private:
  struct stable_compare {
    Compare compare;
    // sequence ordering must be inverted for max heaps
    // so earlier (smaller) insertions come before later
    bool operator()(const value_type &a, const value_type &b) const {
      return compare(a.first, b.first)
        || (!compare(b.first, a.first) && !(a.second < b.second));
    }
  };

 public:
  stable_priority_queue() = default;

  explicit stable_priority_queue(Compare compare) :
    compare_ {compare} {}

  // Queue methods ----------------------------------------------------------

 public:
  bool empty() const { return heap_.empty(); }
  size_t size() const { return heap_.size(); }

 public:
  const Type &top() const { return heap_.front().first; }

 public:
  void push(const Type &value) {
    heap_.emplace_back(value, sequence_);
    std::push_heap(heap_.begin(), heap_.end(), compare_);
    sequence_++; assert(sequence_ && "Sequence overflow");
  }

  void push(Type &&value) {
    heap_.emplace_back(std::move(value), sequence_);
    std::push_heap(heap_.begin(), heap_.end(), compare_);
    sequence_++; assert(sequence_ && "Sequence overflow");
  }

 public:
  void pop() {
    std::pop_heap(heap_.begin(), heap_.end(), compare_);
    heap_.pop_back();
  }

 public:
  void swap(stable_priority_queue<Type> &that) {
    using std::swap;
    swap(sequence_, that.sequence_);
    swap(compare_, that.compare_);
    swap(heap_, that.heap_);
  }

  // SequenceContainer methods ----------------------------------------------

 public:
  const Type &front() const { return heap_.front().first; }

 public:
  const_iterator begin() const { return heap_.begin(); }
  const_iterator end() const { return heap_.end(); }
  iterator begin() { return heap_.begin(); }
  iterator end() { return heap_.end(); }

 public:
  void push_back(const Type &value) { push(value); }
  void push_back(Type &&value) { push(std::move(value)); }
  void pop_front() { pop(); }

 public:
  void erase(const_iterator iter) {
    heap_.erase(iter);
    std::make_heap(heap_.begin(), heap_.end(), compare_);
  }

  void erase(const_iterator iter, const_iterator end) {
    heap_.erase(iter, end);
    std::make_heap(heap_.begin(), heap_.end(), compare_);
  }

  void clear() {
    heap_.clear();
  }

 private:
  std::vector<value_type> heap_;
  stable_compare compare_;
  uint64_t sequence_ = 0;
};

}  // namespace core

#endif
