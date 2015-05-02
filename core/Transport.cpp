
#include "core/common.hpp"
#include "core/Transport.hpp"

namespace core {

Transport::Transport() {
  when(OnMessageSent, [this](auto buffer) {
        this->Send(buffer);
      });

  when(OnMessageReceived, [this](auto buffer) {
        this->Receive(buffer);
      });
}

}  // namespace core
