#include <iostream>
#include <optional>
#include <variant>

struct StateBase {
  void on_enter() {}
  void on_exit() {}
};

template <typename... States>
class FiniteStateMachine {
 public:
  template <typename State>
  static auto transition_to(State state) {
    return std::optional<std::variant<States...>>{std::move(state)};
  }

  static auto remain() {  //
    return std::optional<std::variant<States...>>{std::nullopt};
  }

  template <typename EventType>
  std::size_t operator()(const EventType& event) {
    auto maybe_next_state = std::visit(
      [this, &event](auto&& curr_state) {
        return curr_state(state_, event);  //
      },
      state_);
    if (maybe_next_state) {
      std::visit([](auto&& s) { s.on_exit(); }, state_);
      state_ = std::move(maybe_next_state).value();
      std::visit([](auto&& s) { s.on_enter(); }, state_);
    }
    return state_.index();
  }

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
  auto operator()(auto&& state, const ConnectionEvent& event);
  auto operator()(auto&& state, const ShutdownEvent& event);
  auto operator()(auto&& state, const auto& ignore);
};
struct AppReady {
  void on_enter() { std::cout << "AppReady entered.\n"; }
  void on_exit() { std::cout << "AppReady exited.\n"; }
  auto operator()(auto&& state, const ScenarioEvent& event);
  auto operator()(auto&& state, const ShutdownEvent& event);
  auto operator()(auto&& state, const auto& ignore);
};
struct AppShutdown {
  void on_enter() { std::cout << "AppShutdown entered.\n"; }
  void on_exit() { std::cout << "AppShutdown exited.\n"; }
  auto operator()(auto&& state, const auto& ignore);
};
struct SimReady {
  void on_enter() { std::cout << "SimReady entered.\n"; }
  void on_exit() { std::cout << "SimReady exited.\n"; }
  auto operator()(auto&& state, const ShutdownEvent& event);
  auto operator()(auto&& state, const auto& ignore);
};
struct SimStart {
  void on_enter() { std::cout << "SimStart entered.\n"; }
  void on_exit() { std::cout << "SimStart exited.\n"; }
  auto operator()(auto&& state, const auto& ignore);
};
struct SimPause {
  void on_enter() { std::cout << "SimPause entered.\n"; }
  void on_exit() { std::cout << "SimPause exited.\n"; }
  auto operator()(auto&& state, const auto& ignore);
};
struct SimStop {
  void on_enter() { std::cout << "SimStop entered.\n"; }
  void on_exit() { std::cout << "SimStop exited.\n"; }
  auto operator()(auto&& state, const auto& ignore);
};

using MyStateMachine = FiniteStateMachine<  //
  AppStart,
  AppReady,
  AppShutdown,
  SimReady,
  SimStart,
  SimPause,
  SimStop>;

auto AppStart::operator()(auto&& state, const ConnectionEvent& event) {
  if (event.app_shutdown_count) {
    return MyStateMachine::transition_to(AppShutdown{});
  }
  if (event.app_start_count == kAppReadyThreshold) {
    return MyStateMachine::transition_to(AppReady{});
  }
  return MyStateMachine::remain();
}

auto AppStart::operator()(auto&& state, const ShutdownEvent&) {
  return MyStateMachine::transition_to(AppShutdown{});
}

auto AppStart::operator()(auto&& state, const auto& ignore) {
  return MyStateMachine::remain();
}

auto AppReady::operator()(auto&& state, const ScenarioEvent&) {
  return MyStateMachine::transition_to(SimReady{});
}

auto AppReady::operator()(auto&& state, const ShutdownEvent&) {
  return MyStateMachine::transition_to(AppShutdown{});
}

auto AppReady::operator()(auto&& state, const auto&) {
  return MyStateMachine::remain();
}

auto AppShutdown::operator()(auto&& state, const auto&) {
  return MyStateMachine::remain();
}

auto SimReady::operator()(auto&& state, const ShutdownEvent&) {
  return MyStateMachine::transition_to(AppShutdown{});
}

auto SimReady::operator()(auto&& state, const auto&) {
  return MyStateMachine::remain();
}

auto SimPause::operator()(auto&& state, const auto&) {
  return MyStateMachine::remain();
}

auto SimStart::operator()(auto&& state, const auto&) {
  return MyStateMachine::remain();
}

auto SimStop::operator()(auto&& state, const auto&) {
  return MyStateMachine::remain();
}

int main() {
  MyStateMachine sm;
  sm(ConnectionEvent{.app_start_count = 0});
  sm(ConnectionEvent{.app_start_count = 1});
  sm(ConnectionEvent{.app_start_count = 2});
  sm(ConnectionEvent{.app_start_count = 3});
  sm(ScenarioEvent{});
  sm(ShutdownEvent{});
}
