#ifndef SRC_CORE_EXECUTOR_HPP_
#define SRC_CORE_EXECUTOR_HPP_

#include <chrono>

#include "core/Handle.hpp"

namespace core {

class Executor {
 public:
  using Work = std::function<void()>;
  using Duration = std::chrono::milliseconds;
  using Handle = core::Handle<uint64_t>;

 public:
  virtual ~Executor() {}
  virtual bool Remove(Handle handle) = 0;
  virtual Handle Schedule(Work item, Duration expiry = {}, Duration period = {}) = 0;
};

}  // namespace core

#endif
