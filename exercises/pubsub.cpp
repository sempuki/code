#include <functional>
#include <iostream>
#include <vector>

bool quit = false;

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
  std::string name;
  int state = 0;
};

struct C1 final : public Component {
  void tick() override {
    switch (state) {
      case 0: {
        publisher_m1.publish(M1{});
        state++;
      } break;
      case 1: {
        publisher_m2.publish(M2{});
        state++;
      } break;
      default: break;
    }
  }

  Publisher<M1> publisher_m1;
  Publisher<M2> publisher_m2;
};

struct C2 final : public Component {
  void connect(Publisher<M1>* p1, Publisher<M2>* p2) {
    p1->subscribers.push_back([this](M1) {
      std::cout << name << " received M1\n";
      state++;
    });
    p2->subscribers.push_back([this](M2) {
      std::cout << name << " received M2\n";
      state++;
    });
  }

  void tick() override {
    switch (state) {
      case 2: {
        publisher_m3.publish(M3{});
      } break;
      default: break;
    }
  }

  Publisher<M3> publisher_m3;
};

struct C3 final : public Component {
  void connect(Publisher<M3>* p3) {
    p3->subscribers.push_back([this](M3) {
      std::cout << name << " received M3\n";
      quit = true;
    });
  }

  void tick() override {}
};

int main() {
  C1 c1;
  C2 c2;
  C3 c3;

  c2.name = "C2";
  c3.name = "C3";

  c2.connect(&c1.publisher_m1, &c1.publisher_m2);
  c3.connect(&c2.publisher_m3);

  std::vector<Component*> components{&c1, &c2, &c3};

  while (!quit) {
    for (auto c : components) {
      c->tick();
    }
  }
}
