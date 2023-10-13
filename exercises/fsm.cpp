#include <iostream>
#include <variant>

template <typename... States>
class FiniteStateMachine {
 public:
  FiniteStateMachine() {
    std::visit([](auto&& s) { s.on_enter(); }, state_);
  }

  ~FiniteStateMachine() {
    std::visit([](auto&& s) { s.on_exit(); }, state_);
  }

  template <typename NextStateType, typename... NextStateArgs>
  static auto transition_to(NextStateArgs&&... args) {
    return [... args = std::forward<NextStateArgs>(args)](
               std::variant<States...>& state) mutable {
      std::visit([](auto&& s) { s.on_exit(); }, state);
      state.template emplace<NextStateType>(std::move(args)...);
      std::visit([](auto&& s) { s.on_enter(); }, state);
    };
  }
  static auto transition_none() {
    return [](std::variant<States...>&) {};
  }

  template <typename EventType>
  std::size_t operator()(EventType&& event) {
    std::visit(
        [this, event = std::forward<EventType>(event)](
            auto&& current_alternative) mutable {
          current_alternative(std::move(event))(state_);  //
        },
        state_);
    return state_.index();
  }

  std::size_t state_index() const { return state_.index(); }
  auto visit_state(auto&& visitor) const { return std::visit(visitor, state_); }

 private:
  std::variant<States...> state_;
};

struct ConnectionEvent {
  int app_start_count = 0;
  int app_shutdown_count = 0;
};

struct ScenarioEvent {};
struct ShutdownEvent {};

struct AppStart {
  int kAppReadyThreshold = 3;
  void on_enter() { std::cout << "AppStart entered.\n"; }
  void on_exit() { std::cout << "AppStart exited.\n"; }
  auto operator()(const ConnectionEvent& event);
  auto operator()(const ShutdownEvent& event);
  auto operator()(const auto& ignore);
};
struct AppReady {
  void on_enter() { std::cout << "AppReady entered.\n"; }
  void on_exit() { std::cout << "AppReady exited.\n"; }
  auto operator()(const ScenarioEvent& event);
  auto operator()(const ShutdownEvent& event);
  auto operator()(const auto& ignore);
};
struct AppShutdown {
  void on_enter() { std::cout << "AppShutdown entered.\n"; }
  void on_exit() { std::cout << "AppShutdown exited.\n"; }
  auto operator()(const auto& ignore);
};
struct SimReady {
  void on_enter() { std::cout << "SimReady entered.\n"; }
  void on_exit() { std::cout << "SimReady exited.\n"; }
  auto operator()(const ShutdownEvent& event);
  auto operator()(const auto& ignore);
};
struct SimStart {
  void on_enter() { std::cout << "SimStart entered.\n"; }
  void on_exit() { std::cout << "SimStart exited.\n"; }
  auto operator()(const auto& ignore);
};
struct SimPause {
  void on_enter() { std::cout << "SimPause entered.\n"; }
  void on_exit() { std::cout << "SimPause exited.\n"; }
  auto operator()(const auto& ignore);
};
struct SimStop {
  void on_enter() { std::cout << "SimStop entered.\n"; }
  void on_exit() { std::cout << "SimStop exited.\n"; }
  auto operator()(const auto& ignore);
};

using MyStateMachine = FiniteStateMachine<  //
    AppStart, AppReady, AppShutdown, SimReady, SimStart, SimPause, SimStop>;

auto AppStart::operator()(const ConnectionEvent& event) {
  if (event.app_shutdown_count) {
    return MyStateMachine::transition_to<AppShutdown>();
  }
  if (event.app_start_count == kAppReadyThreshold) {
    return MyStateMachine::transition_to<AppReady>();
  }
  return MyStateMachine::transition_none();
}

auto AppStart::operator()(const ShutdownEvent&) {
  return MyStateMachine::transition_to<AppShutdown>();
}

auto AppStart::operator()(const auto& ignore) {
  return MyStateMachine::transition_none();
}

auto AppReady::operator()(const ScenarioEvent&) {
  return MyStateMachine::transition_to<SimReady>();
}

auto AppReady::operator()(const ShutdownEvent&) {
  return MyStateMachine::transition_to<AppShutdown>();
}

auto AppReady::operator()(const auto&) {
  return MyStateMachine::transition_none();
}

auto AppShutdown::operator()(const auto&) {
  return MyStateMachine::transition_none();
}

auto SimReady::operator()(const ShutdownEvent&) {
  return MyStateMachine::transition_to<AppShutdown>();
}

auto SimReady::operator()(const auto&) {
  return MyStateMachine::transition_none();
}

auto SimPause::operator()(const auto&) {
  return MyStateMachine::transition_none();
}

auto SimStart::operator()(const auto&) {
  return MyStateMachine::transition_none();
}

auto SimStop::operator()(const auto&) {
  return MyStateMachine::transition_none();
}

int main() {
  MyStateMachine sm;
  sm(ConnectionEvent{.app_start_count = 0});
  sm(ConnectionEvent{.app_start_count = 1});
  sm(ConnectionEvent{.app_start_count = 2});
  sm(ConnectionEvent{.app_start_count = 3});
  sm(ScenarioEvent{});
  sm(ShutdownEvent{});
}:
