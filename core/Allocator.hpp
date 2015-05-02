#ifndef SRC_CORE_ALLOCATOR_HPP_
#define SRC_CORE_ALLOCATOR_HPP_

#include "core/Buffer.hpp"

namespace core {

class Allocator {
 public:
  virtual ~Allocator() {}
  virtual UniqueByteBuffer Allocate(size_t bytes) = 0;
};

class DefaultAllocator : public Allocator {
 public:
  UniqueByteBuffer Allocate(size_t bytes) override;
};

}  // namespace core

#endif
