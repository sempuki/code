#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <typeindex>
#include <vector>

// Solution 1: Basics

struct M1 {};
struct M2 {};
struct M3 {};

template <typename M>
struct Publisher {
  void publish(M m) {
    for (auto&& s : subscribers) {
      s(m);
    }
  }

  std::vector<std::function<void(M)>> subscribers;
};

struct Component {
  virtual void tick() = 0;
};

struct C1 final : public Component {
  void tick() override {
    publisher_m1.publish(M1{});
    publisher_m2.publish(M2{});
  }

  Publisher<M1> publisher_m1;
  Publisher<M2> publisher_m2;
};

struct C2 final : public Component {
  void connect(Publisher<M1>* p1, Publisher<M2>* p2) {
    p1->subscribers.push_back([this](M1 m) { std::cout << "M1\n"; });
    p2->subscribers.push_back([this](M2 m) { std::cout << "M2\n"; });
  }

  void tick() override { publisher_m3.publish(M3{}); }

  Publisher<M3> publisher_m3;
};

struct C3 final : public Component {
  void connect(Publisher<M3>* p3) {
    p3->subscribers.push_back([this](M3 m) { std::cout << "M3\n"; });
  }

  void tick() override {
    // ...
  }
};

// Solution 2: Broker

struct BasePublisher {
  virtual void publish_generic(void* m) = 0;
};

template <typename M>
struct TypedPublisher final : public BasePublisher {
  void publish_generic(void* m) { publish(*reinterpret_cast<M*>(m)); }
  void publish(M m) {
    for (auto&& s : subscribers) {
      s(m);
    }
  }

  std::vector<std::function<void(M)>> subscribers;
};

class Broker {
 public:
  template <typename M>
  void publish(M m) {
    publishers_[typeid(M)]->publish_generic(&m);
  }

  template <typename M>
  void subscribe(std::vector<std::function<void(M)>> s) {
    auto p = std::make_unique<TypedPublisher<M>>();
    p->subscribers = std::move(s);
    publishers_[typeid(M)] = std::move(p);
  }

 private:
  std::map<std::type_index, std::unique_ptr<BasePublisher>> publishers_;
};

void do_broker() {
  Broker broker;
  broker.subscribe<M1>({[](M1 m) { std::cout << "M1\n"; }});
  broker.subscribe<M2>({[](M2 m) { std::cout << "M2\n"; }});
  broker.subscribe<M3>({[](M3 m) { std::cout << "M3\n"; }});

  broker.publish(M1{});
  broker.publish(M2{});
  broker.publish(M3{});
}

int main() {
  C1 c1;
  C2 c2;
  C3 c3;

  c2.connect(&c1.publisher_m1, &c1.publisher_m2);
  c3.connect(&c2.publisher_m3);

  std::vector<Component*> components{&c1, &c2, &c3};
  for (auto c : components) {
    c->tick();
  }

  do_broker();
}
