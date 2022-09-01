#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

template <typename Key, typename Value, size_t Capacity>
class FlatLruCache {
 public:
  Value& operator[](const Key& key) {
    size_t index = 0;
    const auto iter = std::find(keys_.begin(), keys_.end(), key);
    if ((iter == keys_.end()) || (index = std::distance(keys_.begin(), iter), times_[index] == 0)) {
      index = std::distance(times_.begin(), std::min_element(times_.begin(), times_.end()));
      keys_[index] = key;
    }
    times_[index]++;
    return values_[index];
  }

  bool has(const Key& key) {
    const auto iter = std::find(keys_.begin(), keys_.end(), key);
    return iter != keys_.end() && times_[std::distance(keys_.begin(), iter)] != 0;
  }

  bool invalidate(const Key& key) {
    const auto iter = std::find(keys_.begin(), keys_.end(), key);
    if (iter != keys_.end()) {
      size_t index = std::distance(keys_.begin(), iter);
      times_[index] = 0;
      values_[index] = {};
      return true;
    }
    return false;
  }

 private:
  std::array<uint64_t, Capacity> times_{};
  std::array<Key, Capacity> keys_{};
  std::array<Value, Capacity> values_{};
};

template <typename Key, typename Value, size_t Capacity>
class OrderedLruCache {
 public:
  Value& operator[](const Key& key) {
    const auto iter = keys_.find(key);
    if (iter != keys_.end()) {
      values_.splice(values_.begin(), values_, iter);
      return *iter;
    } else {
      if (values_.size() == Capacity) {
        keys_.erase();
      }
      values_.emplace_front();
      keys_[key] = values_.begin();
      return values_.front();
    }
  }

  bool has(const Key& key) {
    return keys_.find(key) != keys_.end();  //
  }

  bool invalidate(const Key& key) {
    //
  }

 private:
  std::unordered_map<Key, typename std::list<Value>::iterator> keys_;
  std::list<Value> values_;
};

int main() {
  using T = FlatLruCache<std::string, int, 3>;
  {
    T cache;
    {  // It should have a key for value.
      cache["k"] = 5;
      assert(cache.has("k"));
    }
    {  // It should have the value for key.
      assert(cache["k"] == 5);
    }
    {  // It can invalidate key.
      assert(cache.invalidate("k"));
    }
    {  // It should not have value for invalid key.
      assert(!cache.has("k"));
    }
  }
}
