#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <tuple>
#include <valarray>
#include <vector>

template <typename T, size_t N>
class Vector : public std::valarray<T> {
 public:
  Vector() : std::valarray<T>(0.0, N) {}
  using std::valarray<T>::valarray;
  using std::valarray<T>::operator=;

  double norm() const {
    return std::sqrt(std::accumulate(std::cbegin(*this), std::cend(*this), 0.0,
                                     [](auto s, auto v) { return s + v * v; }));
  }

 private:
  friend std::ostream& operator<<(std::ostream& out, const Vector& vec) {
    size_t n = vec.size();
    out << '[';
    for (auto v : vec) {
      out << ' ' << v << (--n ? ',' : ' ');
    }
    out << ']';
    return out;
  }
};

using Vec2 = Vector<double, 2>;
using Vec3 = Vector<double, 3>;
constexpr size_t X = 0;
constexpr size_t Y = 1;
constexpr size_t Z = 2;
constexpr double EPSILON = 0.0001;

bool is_equal(double x, double y) { return std::abs(x - y) < EPSILON; }

class Name {
 public:
  Name(uintptr_t name) : name_{name} {}
  bool operator==(const Name& that) const { return name_ == that.name_; }
  bool operator!=(const Name& that) const { return name_ != that.name_; }
  bool operator<(const Name& that) const { return name_ < that.name_; }

 private:
  uintptr_t name_ = 0;
};

class Identity {
 public:
  Identity() : id_{std::make_unique<char>()} {}
  Name name() const { return Name{reinterpret_cast<uintptr_t>(id_.get())}; }
  bool operator==(const Identity& that) const { return name() == that.name(); }
  bool operator!=(const Identity& that) const { return name() != that.name(); }
  bool operator<(const Identity& that) const { return name() < that.name(); }

 private:
  std::unique_ptr<char> id_;
};

struct ComponentName final : public Name {
  ComponentName(Name name) : Name{name} {}
};
struct EventName final : public Name {
  EventName(Name name) : Name{name} {}
};
struct EntityName final : public Name {
  EntityName(Name name) : Name{name} {}
};

struct PerObjectIdentity {
  Identity id_;
};

template <typename Type>
struct PerTypeIdentity {
  static Identity id_;
};

template <typename Type>
Identity PerTypeIdentity<Type>::id_;

class ComponentBase {
 public:
  virtual ComponentName component_name() const = 0;
};

class EventBase {
 public:
  virtual double time() const = 0;
  virtual EventName event_name() const = 0;
};

template <typename ComponentType>
class Component : public ComponentBase, public PerTypeIdentity<ComponentType> {
 public:
  ComponentName component_name() const override { return Component::name(); }
  static ComponentName name() {
    return PerTypeIdentity<ComponentType>::id_.name();
  }
};

template <typename MessageType>
class Event final : public EventBase, public PerTypeIdentity<MessageType> {
 public:
  template <typename... Args>
  Event(double time, Args&&... args)
      : time_{time}, message_{std::forward<Args>(args)...} {}

  const MessageType& data() const { return message_; }
  double time() const override { return time_; }
  EventName event_name() const override { return Event::name(); }
  static EventName name() { return PerTypeIdentity<MessageType>::id_.name(); }

 private:
  double time_ = 0.0;
  MessageType message_;

  friend bool operator<(const Event& a, const Event& b) {
    return a.time_ < b.time_;
  }
};

class Entity : public PerObjectIdentity {
 public:
  EntityName entity_name() const { return id_.name(); }
  void attach(ComponentName name, ComponentBase* component) {
    components_[name] = component;
  }

  template <typename ComponentType>
  ComponentType* component() {
    auto iter = components_.find(ComponentType::name());
    return iter != components_.end()
               ? dynamic_cast<ComponentType*>(iter->second)
               : nullptr;
  }

 private:
  std::map<Name, ComponentBase*> components_;
};

class EventQueue final {
 public:
  template <typename HandlerType>
  void start_timer(double time, HandlerType&& handler) {
    handlers_[Event<Timer>::name()].emplace_back(
        [](double time, const EventBase* base) {
          auto* event = dynamic_cast<const Event<Timer>*>(base);
          event->data().action(time);
        });
    events_.emplace(std::make_unique<Event<Timer>>(
        time, std::forward<HandlerType>(handler)));
  }

  template <typename MessageType, typename HandlerType>
  void subscribe(HandlerType&& handler) {
    handlers_[Event<MessageType>::name()].emplace_back(
        [msg_handler = std::forward<HandlerType>(handler)](
            double time, const EventBase* base) {
          auto* event = dynamic_cast<const Event<MessageType>*>(base);
          msg_handler(time, event->data());
        });
  }

  template <typename MessageType>
  void publish(double time, MessageType&& message) {
    events_.emplace(std::make_unique<Event<MessageType>>(
        time, std::forward<MessageType>(message)));
  }

  void process_until(double time) {
    while (events_.size() && events_.top()->time() <= time) {
      for (auto& handler : handlers_[events_.top()->event_name()]) {
        handler(time, events_.top().get());
      }
      events_.pop();
    }
  }

 private:
  struct Timer final {
    std::function<void(double)> action;
  };

  struct Compare final {
    bool operator()(const std::unique_ptr<EventBase>& a,
                    const std::unique_ptr<EventBase>& b) {
      return a->time() < b->time();
    }
  };

  std::priority_queue<std::unique_ptr<EventBase>,
                      std::vector<std::unique_ptr<EventBase>>, Compare>
      events_;
  std::map<EventName,
           std::vector<std::function<void(double, const EventBase*)>>>
      handlers_;
};

class ComponentSystemBase {
 public:
  virtual ComponentName component_name() const = 0;
};

template <typename ComponentType>
class TypedComponentSystemBase : public ComponentSystemBase {
 public:
  ComponentName component_name() const override {
    return ComponentType::name();
  }

  ComponentType* attach(Entity* entity) {
    components_.emplace_back(std::make_unique<ComponentType>());
    auto* component = components_.back().get();
    entity->attach(ComponentType::name(), component);
    return component;
  }

 protected:
  std::vector<std::unique_ptr<ComponentType>> components_;
};

template <typename ComponentType, typename ComputationType>
struct System final : public TypedComponentSystemBase<ComponentType> {
  System() : compute_{ComputationType{}} {}

  template <typename StatefulComputation>
  System(StatefulComputation&& compute_)
      : compute_{std::forward<StatefulComputation>(compute_)} {}

  void operator()(double time, double step, EventQueue* events) {
    for (auto&& component : this->components_) {
      compute_.initialize();
    }

    for (auto&& component : this->components_) {
      compute_(component.get(), time, step, events);
    }

    for (auto&& component : this->components_) {
      compute_.finalize();
    }
  }

  ComputationType compute_;
};

template <typename ComponentType>
struct ComputeBase {
  void initialize() {}
  void finalize() {}
  void operator()(ComponentType* component, double time, double step,
                  EventQueue* events) {}
};

struct Environment;
struct Physical;
struct Controls;
struct Movement;

struct Environment final : public Component<Environment> {
  Vec3 wind;
};

struct Intelligent final : public Component<Intelligent> {
  double iq = 0.0;
};

struct Physical final : public Component<Physical> {
  double radius;
  double wind_resistance_factor = 1.0;
  Movement* movement = nullptr;
};

struct Controls final : public Component<Controls> {
  Vec3 acceleration;
  Intelligent* intelligent = nullptr;
};

struct Movement final : public Component<Movement> {
  Vec3 position;
  Vec3 velocity;

  Environment* environment = nullptr;
  Physical* physical = nullptr;
  Controls* controls = nullptr;
};

namespace detail {
void relate(Controls* controls, Intelligent* intelligent) {
  if (controls) {
    controls->intelligent = intelligent;
  }
}
void relate(Movement* movement, Environment* environment) {
  if (movement) {
    movement->environment = environment;
  }
}
void relate(Movement* movement, Physical* physical) {
  if (movement) {
    movement->physical = physical;
  }
  if (physical) {
    physical->movement = movement;
  }
}
void relate(Movement* movement, Controls* controls) {
  if (movement) {
    movement->controls = controls;
  }
}
}  // namespace detail

struct Collision final {
  Physical* a = nullptr;
  Physical* b = nullptr;
};

auto compute_acceleration(Movement* m) {
  return m->controls->acceleration + (m->physical->wind_resistance_factor *
                                      (m->environment->wind - m->velocity));
}

bool has_collision(Physical* a, Physical* b) {
  auto distance =
      static_cast<Vec3>(a->movement->position - b->movement->position).norm();
  return distance <= (a->radius + b->radius);
}

struct DetectSphericalCollision : public ComputeBase<DetectSphericalCollision> {
  void initialize() { others.clear(); }
  void operator()(Physical* current, double time, double step,
                  EventQueue* events) {
    for (auto previous : others) {
      if (has_collision(previous, current)) {
        events->publish(time, Collision{previous, current});
      }
    }
    others.push_back(current);
  }
  std::vector<Physical*> others;
};

struct ComputeControls : public ComputeBase<ComputeControls> {
  void operator()(Controls* controls, double time, double step,
                  EventQueue* events) {
    if (controls->intelligent) {
      controls->acceleration *= controls->intelligent->iq;
    }
  }
};

struct ForwardEulerIntegration : public ComputeBase<ForwardEulerIntegration> {
  void operator()(Movement* movement, double time, double step,
                  EventQueue* events) {
    auto prev = *movement;
    auto& next = *movement;
    auto acceleration = compute_acceleration(movement);

    next.velocity = prev.velocity + acceleration * step;
    next.position = prev.position + prev.velocity * step;
  }
};

struct TrapezoidIntegration : public ComputeBase<TrapezoidIntegration> {
  void operator()(Movement* movement, double time, double step,
                  EventQueue* events) {
    auto prev = *movement;
    auto& next = *movement;
    auto acceleration = compute_acceleration(movement);

    next.velocity = prev.velocity + acceleration * step;
    next.position =
        prev.position + (prev.velocity + next.velocity) * step * 0.5;
  }
};

struct RungeKutta2Integration : public ComputeBase<RungeKutta2Integration> {
  void operator()(Movement* movement, double time, double step,
                  EventQueue* events) {
    auto prev = *movement;
    auto& next = *movement;
    auto acceleration = compute_acceleration(movement);

    // k1.velocity = prev.velocity + acceleration * step;
    // mid.velocity = prev.velocity + k1.velocity * step * 0.5;
    // k2.velocity = mid.velocity + acceleration * step;

    next.velocity = prev.velocity + acceleration * step;
    next.position =
        prev.position +
        ((prev.velocity + (prev.velocity + acceleration * step) * step * 0.5) +
         acceleration * step) *
            step;
  }
};

struct Actor : public Entity {};

struct ComputeMovement : public RungeKutta2Integration {};
constexpr double STEP_SIZE = 0.1;
constexpr double SUB_STEP_FACTOR = 0.1;

class Simulation final {
 public:
  template <typename ComponentType>
  ComponentType* imbue(Actor* actor) {
    return nullptr;
  }

  Actor* find(EntityName name);
  std::tuple<Environment*, Intelligent*, Physical*, Controls*, Movement*>
  insert(Actor actor);

  void operator()(double time, double step) {
    static auto do_substep = [](auto& system, double time, double step,
                                EventQueue* events) {
      for (double substep = step * SUB_STEP_FACTOR, start = time;
           time < start + step; time += substep) {
        system(time, substep, events);
      }
    };

    do_substep(environment_, time, step, &events);
    do_substep(intelligent_, time, step, &events);
    do_substep(physical_, time, step, &events);
    do_substep(controls_, time, step, &events);
    do_substep(movement_, time, step, &events);
    events.process_until(time);
  }

  EventQueue events;

 private:
  System<Environment, ComputeBase<Environment>> environment_;
  System<Physical, DetectSphericalCollision> physical_;
  System<Intelligent, ComputeBase<Intelligent>> intelligent_;
  System<Controls, ComputeControls> controls_;
  System<Movement, ComputeMovement> movement_;
  std::vector<std::unique_ptr<Actor>> actors_;
};

template <>
Environment* Simulation::imbue<Environment>(Actor* actor) {
  auto* environment = environment_.attach(actor);
  detail::relate(actor->component<Movement>(), environment);
  return environment;
}

template <>
Intelligent* Simulation::imbue<Intelligent>(Actor* actor) {
  auto* intelligent = intelligent_.attach(actor);
  detail::relate(actor->component<Controls>(), intelligent);
  return intelligent;
}

template <>
Physical* Simulation::imbue<Physical>(Actor* actor) {
  auto* physical = physical_.attach(actor);
  detail::relate(actor->component<Movement>(), physical);
  return physical;
}

template <>
Controls* Simulation::imbue<Controls>(Actor* actor) {
  auto* controls = controls_.attach(actor);
  detail::relate(controls, actor->component<Intelligent>());
  detail::relate(actor->component<Movement>(), controls);
  return controls;
}

template <>
Movement* Simulation::imbue<Movement>(Actor* actor) {
  auto* movement = movement_.attach(actor);
  detail::relate(movement, actor->component<Environment>());
  detail::relate(movement, actor->component<Physical>());
  detail::relate(movement, actor->component<Controls>());
  return movement;
}

std::tuple<Environment*, Intelligent*, Physical*, Controls*, Movement*>
Simulation::insert(Actor actor) {
  auto* environment = imbue<Environment>(&actor);
  auto* physical = imbue<Physical>(&actor);
  auto* controls = imbue<Controls>(&actor);
  auto* movement = imbue<Movement>(&actor);
  actors_.emplace_back(std::make_unique<Actor>(std::move(actor)));
  return std::make_tuple(environment, nullptr, physical, controls, movement);
}

Actor* Simulation::find(EntityName name) {
  auto iter = std::find_if(
      actors_.cbegin(), actors_.cend(),
      [name](auto&& actor) { return actor->entity_name() == name; });
  return iter != actors_.cend() ? iter->get() : nullptr;
}

int main() {
  auto ego_actor = Actor{};
  auto obj_actor = Actor{};

  auto ego_name = ego_actor.entity_name();
  auto obj_name = obj_actor.entity_name();

  Simulation simulation;
  auto ego_components = simulation.insert(std::move(ego_actor));
  auto obj_components = simulation.insert(std::move(obj_actor));

  std::get<Environment*>(ego_components)->wind[X] = -12.5;
  std::get<Physical*>(ego_components)->radius = 0.1;
  std::get<Physical*>(ego_components)->wind_resistance_factor = 0.4;
  std::get<Controls*>(ego_components)->acceleration[Y] = -10.0;
  std::get<Movement*>(ego_components)->position[Y] = 100.0;
  std::get<Movement*>(ego_components)->velocity[X] = 10.0;
  std::get<Movement*>(ego_components)->velocity[Z] = 30.0;

  std::get<Physical*>(obj_components)->radius = 1.0;
  std::get<Movement*>(obj_components)->position[X] = -10.0;
  std::get<Movement*>(obj_components)->position[Y] = 36.0;
  std::get<Movement*>(obj_components)->position[Z] = 63.0;

  simulation.events.start_timer(2.0, [ego_name, &simulation](double time) {
    std::cout << "Making Ego intelligent at time " << time << "\n";
    auto* intelligent =
        simulation.imbue<Intelligent>(simulation.find(ego_name));
    intelligent->iq = 1.0;
  });

  bool quit = false;
  simulation.events.subscribe<Collision>(
      [&quit](double time, const Collision& event) {
        std::cout << "Collision at position " << event.a->movement->position
                  << " at time " << time << "\n";
        quit = true;
      });

  for (double time = 0.0, step = STEP_SIZE; time <= 5.0 && !quit;
       time += step) {
    std::cout << "Ego " << std::get<Movement*>(ego_components)->position
              << " at " << time << "\n";
    simulation(time, step);
  }

  std::cout << "Hello world\n";
}
