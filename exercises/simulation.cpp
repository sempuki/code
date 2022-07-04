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

struct ComponentName : public Name {
  ComponentName(Name name) : Name{name} {}
};
struct EventName : public Name {
  EventName(Name name) : Name{name} {}
};
struct EntityName : public Name {
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
class Event : public EventBase, public PerTypeIdentity<MessageType> {
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

struct EventQueue {
  template <typename MessageType, typename HandlerType>
  void subscribe(HandlerType&& handler) {
    handlers[Event<MessageType>::name()].emplace_back(
        [msg_handler =
             std::forward<HandlerType>(handler)](const EventBase* base) {
          auto* event = dynamic_cast<const Event<MessageType>*>(base);
          msg_handler(event->data());
        });
  }

  template <typename MessageType>
  void publish(double time, MessageType&& message) {
    events.emplace(std::make_unique<Event<MessageType>>(
        time, std::forward<MessageType>(message)));
  }

  void process_until(double time) {
    while (events.size() && events.top()->time() <= time) {
      for (auto& handler : handlers[events.top()->event_name()]) {
        handler(events.top().get());
      }
      events.pop();
    }
  }

  struct Compare {
    bool operator()(const std::unique_ptr<EventBase>& a,
                    const std::unique_ptr<EventBase>& b) {
      return a->time() < b->time();
    }
  };

  std::priority_queue<std::unique_ptr<EventBase>,
                      std::vector<std::unique_ptr<EventBase>>, Compare>
      events;
  std::map<EventName, std::vector<std::function<void(const EventBase*)>>>
      handlers;
};

template <typename ComponentType, typename ComputationType>
struct System {
  System() : compute{ComputationType{}} {}

  template <typename StatefulComputation>
  System(StatefulComputation&& compute)
      : compute{std::forward<StatefulComputation>(compute)} {}

  void operator()(double time, double step, EventQueue* events) {
    for (auto&& component : components) {
      compute.initialize();
    }

    for (auto&& component : components) {
      compute(component.get(), time, step, events);
    }

    for (auto&& component : components) {
      compute.finalize();
    }
  }

  void attach(Entity* entity) {
    components.emplace_back(std::make_unique<ComponentType>());
    entity->attach(ComponentType::name(), components.back().get());
  }

  std::vector<std::unique_ptr<ComponentType>> components;
  ComputationType compute;
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

struct Physical final : public Component<Physical> {
  double radius;
  double wind_resistance_factor = 1.0;
  Movement* movement = nullptr;
};

struct Controls final : public Component<Controls> {
  Vec3 acceleration;
};

struct Movement final : public Component<Movement> {
  Vec3 position;
  Vec3 velocity;

  Environment* environment = nullptr;
  Physical* physical = nullptr;
  Controls* controls = nullptr;
};

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

struct Simulation {
  System<Environment, ComputeBase<Environment>> environment;
  System<Physical, DetectSphericalCollision> physical;
  System<Controls, ComputeBase<Controls>> controls;
  System<Movement, ComputeMovement> movement;
  EventQueue events;
  std::vector<Actor> actors;

  std::tuple<Environment*, Physical*, Controls*, Movement*> include(
      Actor actor) {
    environment.attach(&actor);
    physical.attach(&actor);
    controls.attach(&actor);
    movement.attach(&actor);

    auto* environment = actor.component<Environment>();
    auto* physical = actor.component<Physical>();
    auto* controls = actor.component<Controls>();
    auto* movement = actor.component<Movement>();

    physical->movement = movement;
    movement->environment = environment;
    movement->physical = physical;
    movement->controls = controls;

    actors.emplace_back(std::move(actor));
    return std::make_tuple(environment, physical, controls, movement);
  }

  void operator()(double time, double step) {
    static auto do_substep = [](auto& system, double time, double step,
                                EventQueue* events) {
      for (double substep = step * SUB_STEP_FACTOR, start = time;
           time < start + step; time += substep) {
        system(time, substep, events);
      }
    };

    do_substep(environment, time, step, &events);
    do_substep(physical, time, step, &events);
    do_substep(controls, time, step, &events);
    do_substep(movement, time, step, &events);
    events.process_until(time);
  }
};

int main() {
  Simulation simulation;
  auto ego = simulation.include(Actor{});
  auto obj = simulation.include(Actor{});

  std::get<Environment*>(ego)->wind[X] = -12.5;
  std::get<Physical*>(ego)->radius = 0.1;
  std::get<Physical*>(ego)->wind_resistance_factor = 0.4;
  std::get<Controls*>(ego)->acceleration[Y] = -10.0;
  std::get<Movement*>(ego)->position[Y] = 100.0;
  std::get<Movement*>(ego)->velocity[X] = 10.0;
  std::get<Movement*>(ego)->velocity[Z] = 30.0;

  std::get<Physical*>(obj)->radius = 1.0;
  std::get<Movement*>(obj)->position[X] = -10.0;
  std::get<Movement*>(obj)->position[Y] = 36.0;
  std::get<Movement*>(obj)->position[Z] = 63.0;

  bool quit = false;
  simulation.events.subscribe<Collision>([&quit](const Collision& event) {
    std::cout << "Collision at " << event.a->movement->position << "\n";
    quit = true;
  });

  for (double time = 0.0, step = STEP_SIZE; time <= 5.0 && !quit;
       time += step) {
    std::cout << "Ego " << std::get<Movement*>(ego)->position << " at " << time
              << "\n";
    simulation(time, step);
  }

  std::cout << "Hello world\n";
}
