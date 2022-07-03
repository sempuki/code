#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
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
struct EntityName : public Name {
  EntityName(Name name) : Name{name} {}
};

struct EmbueEntityIdentity {
  Identity id_;
};

template <typename ComponentType>
struct EmbueComponentIdentity {
  static Identity id_;
};

template <typename ComponentType>
Identity EmbueComponentIdentity<ComponentType>::id_;

class ComponentBase {
 public:
  virtual ComponentName component_name() const = 0;
};

template <typename ComponentType>
class Component : public ComponentBase,
                  public EmbueComponentIdentity<ComponentType> {
 public:
  ComponentName component_name() const override {
    return ComponentType::id_.name();
  }

  static ComponentName name() { return ComponentType::id_.name(); }
};

class Entity : public EmbueEntityIdentity {
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

template <typename ComponentType, typename ComputationType>
struct System {
  System() : compute{ComputationType{}} {}

  template <typename StatefulComputation>
  System(StatefulComputation&& compute)
      : compute{std::forward<StatefulComputation>(compute)} {}

  void operator()(double time, double step) {
    for (auto&& component : components) {
      compute.initialize();
    }

    for (auto&& component : components) {
      compute(component.get(), time, step);
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

template <typename ComponentType>
struct ComputeBase {
  void initialize() {}
  void finalize() {}
  void operator()(ComponentType* component, double time, double step) {}
};

auto compute_acceleration(Movement* m) {
  return m->controls->acceleration + (m->physical->wind_resistance_factor *
                                      (m->environment->wind - m->velocity));
}

bool is_nearby(Physical* a, Physical* b) {
  auto distance =
      static_cast<Vec3>(a->movement->position - b->movement->position).norm();
  return distance <= (a->radius + b->radius);
}

struct DetectSphericalCollision : public ComputeBase<DetectSphericalCollision> {
  void initialize() { others.clear(); }
  void operator()(Physical* current, double time, double step) {
    for (auto previous : others) {
      if (is_nearby(previous, current)) {
        std::cout << "Collision at " << current->movement->position << "\n";
      }
    }
    others.push_back(current);
  }
  std::vector<Physical*> others;
};

struct ForwardEulerIntegration : public ComputeBase<ForwardEulerIntegration> {
  void operator()(Movement* movement, double time, double step) {
    auto prev = *movement;
    auto& next = *movement;
    auto acceleration = compute_acceleration(movement);

    next.velocity = prev.velocity + acceleration * step;
    next.position = prev.position + prev.velocity * step;
  }
};

struct TrapezoidIntegration : public ComputeBase<TrapezoidIntegration> {
  void operator()(Movement* movement, double time, double step) {
    auto prev = *movement;
    auto& next = *movement;
    auto acceleration = compute_acceleration(movement);

    next.velocity = prev.velocity + acceleration * step;
    next.position =
        prev.position + (prev.velocity + next.velocity) * step * 0.5;
  }
};

struct RungeKutta2Integration : public ComputeBase<RungeKutta2Integration> {
  void operator()(Movement* movement, double time, double step) {
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

  template <typename System>
  void do_substep(System&& system, double time, double step) {
    for (double substep = step * SUB_STEP_FACTOR, start = time;
         time < start + step; time += substep) {
      system(time, substep);
    }
  }

  void operator()(double time, double step) {
    do_substep(environment, time, step);
    do_substep(physical, time, step);
    do_substep(controls, time, step);
    do_substep(movement, time, step);
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

  for (double time = 0.0, step = STEP_SIZE; time <= 5.0; time += step) {
    std::cout << "Ego " << std::get<Movement*>(ego)->position << " at " << time
              << "\n";
    simulation(time, step);
  }

  std::cout << "Hello world\n";
}
