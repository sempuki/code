#ifndef SRC_CORE_STATE_HPP_
#define SRC_CORE_STATE_HPP_

#include "core/common.hpp"
#include "core/Signal.hpp"

namespace core {

template <typename Class, typename Value = typename Class::State>
class State {
 public:
  State() = delete;
  explicit State(Value initial) :
    state_ {initial} {}

 public:
  Value operator()() const { return state_; }

 public:
  core::Signal<Value, Value> Changed;

 private:
  friend Class;
  State &operator=(Value state) {
    if (state_ != state) {
      auto previous = state_;
      auto current = state;

      state_ = state;
      Changed(previous, current);
    }
    return *this;
  }

 private:
  Value state_;
};

}  // namespace core


#endif  // SRC_CORE_STATE_HPP_
