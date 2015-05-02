#ifndef SRC_CORE_TRANSPORT_HPP_
#define SRC_CORE_TRANSPORT_HPP_

#include "core/State.hpp"
#include "core/Signal.hpp"
#include "core/Buffer.hpp"

namespace core {

class Transport {
 public:
  Transport();
  virtual ~Transport() = default;

 public:
  virtual void Send(core::ByteBuffer message) = 0;
  virtual void Receive(core::ByteBuffer message) = 0;

 public:
  core::Signal<core::ByteBuffer> MessageSent, MessageReceived;
  core::Signal<std::error_code> ErrorOccurred;

 public:
  core::Slot<core::ByteBuffer> OnMessageSent, OnMessageReceived;
};

}  // namespace core

#endif
