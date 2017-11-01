#include <iostream>
#include <cstddef>
#include <array>

template <typename Type, size_t N>
class circular_input_iterator {
 public:
  using difference_type = ptrdiff_t;
  using value_type = Type;
  using pointer = Type*;
  using reference = Type&;
  using iterator_category = std::input_iterator_tag;

  circular_input_iterator() = default;
  circular_input_iterator(Type* data, size_t index)
      : data_{data}, index_{index} {}
  circular_input_iterator(const circular_input_iterator&) = default;
  circular_input_iterator& operator=(const circular_input_iterator&) = default;
  ~circular_input_iterator() = default;

  bool operator==(const circular_input_iterator& that) {
    return data_ == that.data_ && index_ == that.index_;
  }

  bool operator!=(const circular_input_iterator& that) {
    return !(*this == that);
  }

  reference operator*() { return *(data_ + (index_ % N)); }
  pointer operator->() { return data_ + (index_ % N); }

  circular_input_iterator& operator++() {
    ++index_;
    return *this;
  }

  circular_input_iterator operator++(int) {
    circular_input_iterator copy{data_, index_};
    ++index_;
    return copy;
  }

 private:
  Type* data_ = nullptr;
  size_t index_ = 0;
};

template <typename Type, size_t N>
class circular_buffer {
 public:
  using const_iterator = circular_input_iterator<const Type, N>;
  using iterator = circular_input_iterator<Type, N>;

  void push(Type sample) {
    data_[total_ % N] = std::move(sample);
    ++total_;
  }

  size_t size() const { return std::min(total_, N); }

  Type& head() { return data_[(total_ - 1) % N]; }
  Type& tail() { return data_[(total_ - size()) % N]; }

  const_iterator begin() const { return {data_.data(), total_ - size()}; }
  const_iterator end() const { return {data_.data(), total_}; }

  iterator begin() { return {data_.data(), total_ - size()}; }
  iterator end() { return {data_.data(), total_}; }

 private:
  std::array<Type, N> data_;
  size_t total_ = 0;
};
