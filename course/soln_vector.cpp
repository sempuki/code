#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

template <typename T>
class vector final {
 public:
  vector() = default;
  ~vector() = default;

  vector(const vector<T> &that)
      : data_{std::make_unique<T[]>(that.capacity_)},
        size_{that.size_},
        capacity_{that.capacity_} {
    std::copy_n(that.data_.get(), that.size_, data_.get());
  }

  vector<T> &operator=(const vector<T> &that) {
    if (this != &that) {
      vector<T> copy{that};
      swap(*this, copy);
    }
    return *this;
  }

  vector(vector<T> &&that) { swap(*this, that); }

  vector<T> &operator=(vector<T> &&that) {
    if (this != &that) {
      swap(*this, that);
    }
    return *this;
  }

  size_t size() const { return size_; }
  size_t capacity() const { return capacity_; }

  T &operator[](size_t i) { return data_[i]; }
  const T &operator[](size_t i) const { return data_[i]; }

  T *begin() { return data_; }
  const T *begin() const { return data_; }

  T *end() { return data_ + size_; }
  const T *end() const { return data_ + size_; }

  T &front() { return data_[0]; }
  const T &front() const { return data_[0]; }

  T &back() { return data_[size_ - 1]; }
  const T &back() const { return data_[size_ - 1]; }

  void push_back(T value) {
    if (size_ == capacity_) {
      reserve(capacity_ * 2);
    }
    data_[size_++] = std::move(value);
  }

  void resize(size_t size) {
    if (size <= size_) {
      for (auto i = size; i < size_; ++i) {
        data_[i].~T();
      }
    } else if (size <= capacity_) {
      for (auto i = size_; i < size; ++i) {
        data_[i] = T();
      }
    } else {
      reserve(size);
    }
    size_ = size;
  }

  void reserve(size_t capacity) {
    if (capacity > capacity_) {
      auto temp = std::make_unique<T[]>(capacity);
      std::copy_n(std::make_move_iterator(data_.get()), size_, temp.get());
      data_ = std::move(temp);
      capacity_ = capacity;
    }
  }

 private:
  friend void swap(vector<T> &a, vector<T> &b) {
    using std::swap;
    swap(a.data_, b.data_);
    swap(a.size_, b.size_);
    swap(a.capacity_, b.capacity_);
  }

  friend bool operator==(const vector<T> &a, const vector<T> &b) {
    return std::equal(a.data_.get(), a.data_.get() + a.size_, b.data_.get(),
                      b.data_.get() + b.size_);
  }

  friend bool operator!=(const vector<T> &a, const vector<T> &b) {
    return !(a == b);
  }

  std::unique_ptr<T[]> data_;
  size_t size_ = 0;
  size_t capacity_ = 0;
};
