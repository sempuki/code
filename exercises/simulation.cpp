#include <cassert>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <valarray>
#include <vector>

template <typename T, size_t N>
class Vector : public std::valarray<T> {
 public:
  Vector() : std::valarray<T>(0.0, N) {}
  using std::valarray<T>::operator=;

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
      compute(component.get(), time, step);
    }
  }

  void attach(Entity* entity) {
    components.emplace_back(std::make_unique<ComponentType>());
    entity->attach(ComponentType::name(), components.back().get());
  }

  std::vector<std::unique_ptr<ComponentType>> components;
  std::function<void(ComponentType*, double, double)> compute;
};

struct Movement final : public Component<Movement> {
  Vec3 position;
  Vec3 velocity;
};

struct ComputeMovement {
  void operator()(Movement* movement, double time, double step) {}
};

struct Controls final : public Component<Controls> {
  Vec3 acceleration;
  Movement* object = nullptr;
};

struct ForwardEulerIntegration {
  void operator()(Controls* control, double time, double step) {
    auto prev = *control->object;
    auto& next = *control->object;

    next.velocity = prev.velocity + control->acceleration * step;
    next.position = prev.position + prev.velocity * step;
  }
};

struct TrapezoidIntegration {
  void operator()(Controls* control, double time, double step) {
    auto prev = *control->object;
    auto& next = *control->object;

    next.velocity = prev.velocity + control->acceleration * step;
    next.position =
        prev.position + (prev.velocity + next.velocity) * step * 0.5;
  }
};

struct RungeKutta2Integration {
  void operator()(Controls* control, double time, double step) {
    auto prev = *control->object;
    auto& next = *control->object;
    Movement k1, k2, mid;

    k1.velocity = prev.velocity + control->acceleration * step;
    mid.velocity = prev.velocity + k1.velocity * step * 0.5;
    k2.velocity = mid.velocity + control->acceleration * step;
    next.velocity = prev.velocity + control->acceleration * step;
    next.position = prev.position + k2.velocity * step;
  }
};

struct Actor : public Entity {};

constexpr double STEP_SIZE = 1.0;
constexpr double SUB_STEP_FACTOR = 1.0;

struct Simulation {
  System<Movement, ComputeMovement> movement;
  System<Controls, TrapezoidIntegration> controls;
  std::vector<Actor> actors;

  template <typename System>
  void do_substep(System&& system, double time, double step) {
    for (double substep = step * SUB_STEP_FACTOR, start = time;
         time < start + step; time += substep) {
      system(time, substep);
    }
  }

  void operator()(double time, double step) {
    do_substep(controls, time, step);
    do_substep(movement, time, step);
  }
};

int main() {
  Actor ego;

  Simulation simulation;
  simulation.movement.attach(&ego);
  simulation.controls.attach(&ego);

  auto* movement = ego.component<Movement>();
  auto* controls = ego.component<Controls>();

  movement->position[Y] = 100.0;
  movement->velocity[X] = 10.0;
  movement->velocity[Z] = 30.0;
  controls->acceleration[Y] = -10.0;
  controls->object = movement;

  simulation.actors.emplace_back(std::move(ego));
  for (double time = 0.0, step = STEP_SIZE; time <= 5.0; time += step) {
    std::cout << "Ego position " << movement->position << " at " << time
              << "\n";
    simulation(time, step);
  }

  std::cout << "Hello world\n";
}
