#ifndef SRC_CORE_SIGNAL_HPP_
#define SRC_CORE_SIGNAL_HPP_

#include "core/common.hpp"

namespace core {

inline namespace signal {
template<typename ...Args> class Slot;
template<typename ...Args> class Signal;

using handle_type = uint8_t;

template <typename ...Args>
class Signal final {
 public:
  Signal() = default;

  Signal(Signal &&) = delete;
  Signal &operator=(Signal &&) = delete;

  Signal(const Signal &) = delete;
  Signal &operator=(const Signal &) = delete;

  ~Signal();

 public:
  size_t connection_count() const;
  size_t once_handler_count() const;
  size_t when_handler_count() const;

 public:
  template<typename ...DeducedArgs>
  void operator()(DeducedArgs &&...args);

 public:
  template<typename Functional>
  handle_type when(Functional &&functional);

  template<typename Functional>
  handle_type once(Functional &&functional);

  void remove(handle_type handle);

 public:
  void disconnect();
  void clear();

 public:
  void connect(Slot<Args...> &slot);  // NOLINT
  void connect(Slot<Args...> *slot);
  void disconnect(Slot<Args...> &slot);  // NOLINT
  void disconnect(Slot<Args...> *slot);

 public:
  void forward(Signal<Args...> &target);  // NOLINT
  void forward(Signal<Args...> *target);
  void disconnect(Signal<Args...> &signal);  // NOLINT
  void disconnect(Signal<Args...> *signal);

 private:
  std::array<std::function<void(Args...)>, 4> handlers_;

 private:
  std::set<handle_type> whenhandles_;
  std::set<handle_type> oncehandles_;
  std::set<Signal<Args...> *> signals_;
  std::set<Slot<Args...> *> slots_;
};

template <typename ...Args>
class Slot final {
 public:
  Slot() = default;

  Slot(const Slot &) = delete;
  Slot &operator=(const Slot &) = delete;

  Slot(Slot &&) = delete;
  Slot &operator=(Slot &&) = delete;

  ~Slot();

 public:
  size_t connection_count() const;
  size_t when_handler_count() const;
  size_t once_handler_count() const;

 public:
  template <typename Functional>
  void when(Functional &&functional);

  template <typename Functional>
  void once(Functional &&functional);

  template <typename Functional>
  void operator*=(Functional &&functional);

  template <typename Functional>
  void operator+=(Functional &&functional);

 public:
  template<typename ...DeducedArgs>
  void operator()(DeducedArgs &&...args);

 public:
  void disconnect();
  void clear();

 public:
  void connect(Signal<Args...> &signal);  // NOLINT
  void connect(Signal<Args...> *signal);
  void disconnect(Signal<Args...> &signal);  // NOLINT
  void disconnect(Signal<Args...> *signal);

 private:
  std::vector<std::function<void(Args...)>> whenhandlers_;
  std::vector<std::function<void(Args...)>> oncehandlers_;

 private:
  std::map<Signal<Args...> *, std::vector<handle_type>> handlemap_;
};


template <typename ...Args, typename Functional>
handle_type when(Signal<Args...> &signal, Functional &&functional) {
  return signal.when(functional);
}

template <typename ...Args, typename Functional>
handle_type once(Signal<Args...> &signal, Functional &&functional) {
  return signal.once(functional);
}

template <typename ...Args, typename Functional>
void when(Slot<Args...> &slot, Functional &&functional) {
  slot.when(functional);
}

template <typename ...Args, typename Functional>
void once(Slot<Args...> &slot, Functional &&functional) {
  slot.once(functional);
}

template <typename ...Args>
void connect(Signal<Args...> &signal, Slot<Args...> &slot) {
  slot.connect(signal);
  signal.connect(slot);
}

template <typename ...Args>
void connect(Slot<Args...> &slot, Signal<Args...> &signal) {
  slot.connect(signal);
  signal.connect(slot);
}

template <typename ...Args>
void disconnect(Signal<Args...> &signal) {
  signal.disconnect();
}

template <typename ...Args>
void disconnect(Slot<Args...> &slot) {
  slot.disconnect();
}

template <typename ...Args>
void disconnect(Signal<Args...> &signal, Slot<Args...> &slot) {
  slot.disconnect(signal);
  signal.disconnect(slot);
}

template <typename ...Args>
void disconnect(Slot<Args...> &slot, Signal<Args...> &signal) {
  slot.disconnect(signal);
  signal.disconnect(slot);
}

namespace {

template <typename Handlers, typename Functional>
handle_type install(Handlers &handlers, Functional &&functional) {  // NOLINT
  auto handler = std::find_if(begin(handlers), end(handlers),
      [](const auto &handler) { return !handler; });

  assert(handler != end(handlers) && "Exceeded maximum active handlers");
  *handler = functional;

  auto index = distance(begin(handlers), handler);
  assert(index <= static_cast<handle_type>(-1) && "Exceeded indexable handle range");

  return static_cast<handle_type>(index);
}

template <typename Handlers>
void erase(Handlers &handlers, handle_type index) {  // NOLINT
  assert(index < handlers.size() && "Exceeded handler bounds");

  auto handler = begin(handlers);
  advance(handler, index);

  *handler = nullptr;
}

}  // namespace


template <typename ...Args>
Signal<Args...>::~Signal() {
  disconnect();
}

template <typename ...Args>
size_t Signal<Args...>::connection_count() const {
  return slots_.size() + signals_.size();
}

template <typename ...Args>
size_t Signal<Args...>::when_handler_count() const {
  return whenhandles_.size();
}

template <typename ...Args>
size_t Signal<Args...>::once_handler_count() const {
  return oncehandles_.size();
}

template <typename ...Args>
template <typename ...DeducedArgs>
void Signal<Args...>::operator()(DeducedArgs &&...args) {
  for (const auto &handler : handlers_) {
    if (handler) {
      handler(args...);
    }
  }
  for (auto signal : signals_) {
    (*signal)(args...);
  }
  for (auto slots : slots_) {
    (*slots)(args...);
  }
  for (auto handle : oncehandles_) {
    erase(handlers_, handle);
  }
  oncehandles_.clear();
}

template <typename ...Args>
template<typename Functional>
handle_type Signal<Args...>::when(Functional &&functional) {
  auto handle = install(handlers_, functional);
  whenhandles_.insert(handle);
  return handle;
}

template <typename ...Args>
template<typename Functional>
handle_type Signal<Args...>::once(Functional &&functional) {
  auto handle = install(handlers_, functional);
  oncehandles_.insert(handle);
  return handle;
}

template <typename ...Args>
void Signal<Args...>::remove(handle_type handle) {
  assert(handle < handlers_.size() && "Invalid handle");
  erase(handlers_, handle);
  whenhandles_.erase(handle);
  oncehandles_.erase(handle);
}

template <typename ...Args>
void Signal<Args...>::disconnect() {
  for (auto slot : slots_) {
    slot->disconnect(this);
  }
  slots_.clear();

  for (auto signal : signals_) {
    signal->disconnect(this);
  }
  signals_.clear();
}

template <typename ...Args>
void Signal<Args...>::clear() {
  disconnect();
  for (auto &handler : handlers_) {
    handler = nullptr;
  }
  whenhandles_.clear();
  oncehandles_.clear();
}

template <typename ...Args>
void Signal<Args...>::connect(Slot<Args...> &slot) {
  connect(&slot);
}

template <typename ...Args>
void Signal<Args...>::connect(Slot<Args...> *slot) {
  slots_.insert(slot);
}

template <typename ...Args>
void Signal<Args...>::disconnect(Slot<Args...> &slot) {
  disconnect(&slot);
}

template <typename ...Args>
void Signal<Args...>::disconnect(Slot<Args...> *slot) {
  slots_.erase(slot);
}

template <typename ...Args>
void Signal<Args...>::forward(Signal<Args...> &target) {
  forward(&target);
}

template <typename ...Args>
void Signal<Args...>::forward(Signal<Args...> *target) {
  signals_.insert(target);
}

template <typename ...Args>
void Signal<Args...>::disconnect(Signal<Args...> &target) {
  disconnect(&target);
}

template <typename ...Args>
void Signal<Args...>::disconnect(Signal<Args...> *target) {
  signals_.erase(target);
}


template <typename ...Args>
Slot<Args...>::~Slot() {
  disconnect();
}

template <typename ...Args>
size_t Slot<Args...>::connection_count() const {
  return handlemap_.size();
}

template <typename ...Args>
size_t Slot<Args...>::when_handler_count() const {
  return whenhandlers_.size();
}

template <typename ...Args>
size_t Slot<Args...>::once_handler_count() const {
  return oncehandlers_.size();
}

template <typename ...Args>
template <typename Functional>
void Slot<Args...>::when(Functional &&functional) {
  for (auto &map : handlemap_) {
    auto signal = map.first;
    auto &handles = map.second;

    handles.push_back(signal->when(functional));
  }

  whenhandlers_.emplace_back(functional);
}

template <typename ...Args>
template <typename Functional>
void Slot<Args...>::once(Functional &&functional) {
  for (auto &map : handlemap_) {
    auto signal = map.first;
    auto &handles = map.second;

    handles.push_back(signal->once(functional));
  }

  oncehandlers_.emplace_back(functional);
}

template <typename ...Args>
template <typename Functional>
void Slot<Args...>::operator*=(Functional &&functional) {
  when(functional);
}

template <typename ...Args>
template <typename Functional>
void Slot<Args...>::operator+=(Functional &&functional) {
  once(functional);
}

template <typename ...Args>
template <typename ...DeducedArgs>
void Slot<Args...>::operator()(DeducedArgs &&...args) {
  oncehandlers_.clear();
}

template <typename ...Args>
void Slot<Args...>::disconnect() {
  for (auto &map : handlemap_) {
    auto signal = map.first;
    for (auto handle : map.second) {
      signal->remove(handle);
    }
    signal->disconnect(this);
  }
}

template <typename ...Args>
void Slot<Args...>::clear() {
  disconnect();
  whenhandlers_.clear();
  oncehandlers_.clear();
}

template <typename ...Args>
void Slot<Args...>::connect(Signal<Args...> &signal) {
  connect(&signal);
}

template <typename ...Args>
void Slot<Args...>::connect(Signal<Args...> *signal) {
  auto &handles = handlemap_[signal];

  for (const auto &handler : whenhandlers_) {
    handles.push_back(signal->when(handler));
  }

  for (const auto &handler : oncehandlers_) {
    handles.push_back(signal->once(handler));
  }
}

template <typename ...Args>
void Slot<Args...>::disconnect(Signal<Args...> &signal) {
  disconnect(&signal);
}

template <typename ...Args>
void Slot<Args...>::disconnect(Signal<Args...> *signal) {
  for (auto handle : handlemap_[signal]) {
    signal->remove(handle);
  }
  handlemap_.erase(signal);
}
}  // namespace signal
}  // namespace core

#endif
