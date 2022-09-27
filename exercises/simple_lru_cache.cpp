#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

template <typename Key, typename Value, size_t Capacity>
class FlatLruCache final {
 public:
  Value& operator[](const Key& key) {
    auto [index, found] = try_find_index(key);
    if (!found) {
      index =
        std::distance(use_count_.begin(), std::min_element(use_count_.begin(), use_count_.end()));
      use_count_[index] = 0;
      keys_[index] = key;
    }
    use_count_[index]++;
    return values_[index];
  }

  bool has(const Key& key) {
    const auto [_, found] = try_find_index(key);
    return found;
  }

  bool invalidate(const Key& key) {
    const auto [index, found] = try_find_index(key);
    if (found) {
      use_count_[index] = 0;
      values_[index] = {};
    }
    return found;
  }

 private:
  std::pair<size_t, bool> try_find_index(const Key& key) {
    const auto iter = std::find(keys_.begin(), keys_.end(), key);
    if (iter != keys_.end()) {
      const auto index = std::distance(keys_.begin(), iter);
      if (use_count_[index] != 0u) {
        return {index, true};
      }
    }
    return {0u, false};
  }

  std::array<Key, Capacity> keys_;
  std::array<Value, Capacity> values_;
  std::array<uint64_t, Capacity>
    use_count_{};  // Zero is a sentinel value that means `unused` or `invalid`.
};

template <typename Key, typename Value, size_t Capacity>
class HashLruCache final {
 public:
  HashLruCache() {
    for (size_t i = 0u; i < Capacity; ++i) {
      used_order_.push_back(&entries_[i]);
      entries_[i].used = std::prev(used_order_.end());
    }
  }

  HashLruCache(const HashLruCache&) = delete;
  HashLruCache& operator=(const HashLruCache&) = delete;

  Value& operator[](const Key& key) {
    const auto iter = keys_.find(key);
    const auto used = (iter != keys_.end())
                        ? iter->second->used
                        : (keys_[key] = used_order_.back(), std::prev(used_order_.end()));

    used_order_.splice(used_order_.begin(), used_order_, used);
    return (*used)->value;
  }

  bool has(const Key& key) { return keys_.find(key) != keys_.end(); }

  bool invalidate(const Key& key) {
    const auto iter = keys_.find(key);
    if (iter != keys_.end()) {
      iter->second->value = {};
      keys_.erase(iter);
      return true;
    }
    return false;
  }

 private:
  struct Entry {
    Value value;
    typename std::list<Entry*>::iterator used;
  };

  std::array<Entry, Capacity> entries_;
  std::unordered_map<Key, Entry*> keys_;
  std::list<Entry*> used_order_;
};

int main() {
  using T = HashLruCache<std::string, int, 3>;
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
