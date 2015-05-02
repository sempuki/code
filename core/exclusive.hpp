#ifndef SRC_ASYNC_EXCLUSIVE_HPP_
#define SRC_ASYNC_EXCLUSIVE_HPP_

namespace async {

// Avoid false sharing of objects by reserving whole cache lines

constexpr size_t cachewidth = 64;
constexpr size_t cacheuse(size_t size) {
  // NOTE: all types are guaranteed to have non-zero size
  return (((size-1) >> core::bits::log2(cachewidth)) + 1) * cachewidth;
}

template <typename Type>
struct exclusive {
  union alignas (cachewidth)
  {
    uint8_t padding[cacheuse(sizeof(Type))];
    Type value;
  };

  template <typename ...Args>
  exclusive(Args &&...args) : value {std::forward<Args>(args)...} {}
  ~exclusive() { value.~Type(); }
};

}  // namespace async

#endif
