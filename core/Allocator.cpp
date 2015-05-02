
#include "core/Allocator.hpp"

namespace core {

core::UniqueByteBuffer DefaultAllocator::Allocate(size_t bytes) {
  return {
    new uint8_t[bytes],
    [](auto buffer) {
      delete [] buffer;
    },
    bytes
  };
}

}  // namespace core
