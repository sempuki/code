#include <atomic>
#include <deque>
#include <iostream>
#include <map>
#include <thread>
#include <utility>
#include <vector>

struct Resource {
    void use() { std::cout << "use Resource" << std::endl; }
};

struct MoveOnlyResource {
    MoveOnlyResource(const MoveOnlyResource&) = delete;
    MoveOnlyResource& operator=(const MoveOnlyResource&) = delete;

    MoveOnlyResource(MoveOnlyResource&&) = default;
    MoveOnlyResource& operator=(MoveOnlyResource&&) = default;

    void use() { std::cout << "use MoveOnlyResource" << std::endl; }
};

struct Actor;
struct Continuation;

struct FiberState {
    FiberState(std::weak_ptr<Actor> actor);

    std::deque<std::function<void(Continuation)>> work_list;
    mutable size_t work_id = 0;
    mutable std::weak_ptr<Actor> host;
    mutable std::atomic<bool> dispose{false};
};

struct Continuation {
    Continuation(std::shared_ptr<FiberState> state);

    std::shared_ptr<const FiberState> state;

    void run_once();
    void run_once_here();
};

struct Fiber {
    Fiber(std::shared_ptr<FiberState> state);

    std::shared_ptr<FiberState> state;

    template <typename Functional>
    Fiber& setup_run(Functional&& work) {
        state->work_list.emplace_front(std::forward<Functional>(work));
        return *this;
    }

    template <typename Functional>
    Fiber& then_do(Functional&& work) {
        state->work_list.emplace_back(std::forward<Functional>(work));
        return *this;
    }

    Continuation start_run();
};

struct Actor : std::enable_shared_from_this<Actor> {
    ~Actor();

    Fiber create_fiber();

    template <typename Functional>
    void post(Functional&& work) {
        work_list.emplace_back(std::forward<Functional>(work));
    }

    void run_once();

    std::vector<std::shared_ptr<FiberState>> fibers;
    std::deque<std::function<void()>> work_list;
};

FiberState::FiberState(std::weak_ptr<Actor> actor) : host{actor} {}

Fiber::Fiber(std::shared_ptr<FiberState> state) : state{state} {}

Continuation Fiber::start_run() {
    Continuation initial{state};
    state.reset();
    return initial;
}

Continuation::Continuation(std::shared_ptr<FiberState> state) : state{state} {}

void Continuation::run_once() {
    if (auto host = state->host.lock()) {
        host->post([continuation = *this]() mutable {
            continuation.run_once_here();
        });
    }
}

void Continuation::run_once_here() {
    auto next = state->work_id++;
    if (next < state->work_list.size()) {
        state->work_list[next](*this);
    } else {
        state->dispose = true;
    }
}

Actor::~Actor() {
    for (auto&& state : fibers) {
        state->dispose = true;
    }
}

Fiber Actor::create_fiber() {
    auto state = std::make_shared<FiberState>(shared_from_this());
    fibers.push_back(state);
    return Fiber{state};
}

void Actor::run_once() {
    if (work_list.size()) {
        work_list.front()();
        work_list.pop_front();
    }
}

int main() {
    auto&& actor = std::make_shared<Actor>();
    std::thread th{[actor] {
        for (;;) {
            actor->run_once();
        }
    }};

    Fiber fiber = actor->create_fiber();
    fiber
        .setup_run([](auto&& continuation) {
            std::cout << "Set up." << std::endl;
            // Pass resource
            // Resource resource;
        })
        .then_do([](auto&& continuation) {
            std::cout << "Then do 1" << std::endl;
            // Use resource
            Resource resource;
            resource.use();
        });

    auto continuation = fiber.start_run();
    continuation.run_once();
    continuation.run_once();

    th.join();
}
