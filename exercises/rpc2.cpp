#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <deque>

using namespace std::literals::chrono_literals;

template <typename Type> class Channel {
public:
  size_t size() const {
    std::lock_guard<decltype(mtx_)> _{mtx_};
    return queue_.size();
  }

  bool empty() const {
    std::lock_guard<decltype(mtx_)> _{mtx_};
    return queue_.empty();
  }

  void push(Type value) {
    std::lock_guard<decltype(mtx_)> _{mtx_};
    queue_.push_back(value);
  }

  bool pop(Type& value) {
    std::lock_guard<decltype(mtx_)> _{mtx_};
    bool success = !queue_.empty();
    if (success) {
        value = queue_.front();
        queue_.pop_front();
    }
    return success;
  }

private:
  mutable std::mutex mtx_;
  std::deque<Type> queue_;
};

struct Result {
  int32_t error_code = 0;
};

struct User {
  std::string name;
};

struct UserAuth {
  std::string password;
};

struct UserID {
  std::string username;
};

struct UserSession {
  std::string token;
};

struct UserRequest {
  int which = 0;
  UserAuth auth; // 1
  UserID get;    // 2
  UserID ping;   // 3
};

struct UserResponse {
  int which = 0;
  UserSession auth; // 1
  User get;         // 2
  Result ping;      // 3
};

struct Space {
  std::string name;
};

struct SpaceRequest {
  int which = 0;
  UserID join; // 1
};

struct SpaceResponse {
  int which = 0;
  Result join; // 1
};

struct RequestHeader {
  // todo
};

struct Request {
  RequestHeader header;
  int which = 0;
  UserRequest user;   // 1
  SpaceRequest space; // 2
};

struct ResponseHeader {
  // todo
};

struct Response {
  ResponseHeader header;
  int which = 0;
  UserResponse user;   // 1
  SpaceResponse space; // 2
};

struct Packet {
  std::vector<Request> requests;
  std::vector<Response> responses;
};

struct Service {
  template <typename Functional> void post(Functional &&work) {
    work_list.emplace_back(std::forward<Functional>(work));
  }

  void run_once() {
    if (work_list.size()) {
      work_list.front()();
      work_list.pop_front();
    }
  }

  std::deque<std::function<void()>> work_list;
};

struct UserService : public Service,
                     public std::enable_shared_from_this<UserService> {};

struct SpaceService : public Service,
                      public std::enable_shared_from_this<SpaceService> {};

struct Device {
  void send(const Packet &packet) { outgoing.push(packet); };
  bool recv(Packet &packet) { return incoming.pop(packet); };

  Channel<Packet> &outgoing;
  Channel<Packet> &incoming;
};

struct Server {
  void send(const Packet &packet) { outgoing.push(packet); };
  bool recv(Packet &packet) { return incoming.pop(packet); };

  Channel<Packet> &outgoing;
  Channel<Packet> &incoming;
};

int main() {
  std::vector<std::thread> threads;
  std::vector<std::shared_ptr<Service>> services;
  services.emplace_back(std::make_shared<UserService>());
  services.emplace_back(std::make_shared<SpaceService>());

  for (auto service : services) {
    threads.emplace_back(std::thread{[service] {
      for (;;) {
        service->run_once();
      }
    }});
  }

  Channel<Packet> from_device;
  Channel<Packet> from_server;

  std::thread server{[&] { 
      Server server{from_server, from_device};

      Packet packet;
      for (;;) {
          if (server.recv(packet)) {
              for (auto&& request : packet.requests)
              {
                  switch (request.which)
                  {
                  case 1:
                      switch (request.user.which)
                      {
                      case 1:
                          if (request.user.auth.password == "sekret") 
                          {
                              Response response;
                              response.which = 1;
                              response.user.which = 1;
                              response.user.auth.token = "guhjob";

                              Packet packet;
                              packet.responses.push_back(response);
                              server.send(packet);
                          }
                          break;
                      case 2:
                          break;
                      case 3:
                          break;
                      }
                      break;
                  case 2:
                      switch (request.space.which)
                      {
                      case 1:
                          break;
                      }
                      break;
                  }
              }
          }
          std::this_thread::sleep_for(100ms);
      }
  }};

  std::thread device{[&] { 
      Device device{from_device, from_server};

      Request request;
      request.which = 1;
      request.user.which = 1;
      request.user.auth.password = "sekret";

      Packet packet;
      packet.requests.push_back(request);
      device.send(packet);

      for (;;) {
          if (device.recv(packet)) {
              auto& response = packet.responses.back();
              if (response.which == 1 && response.user.which == 1 && response.user.auth.token == "guhjob") {
                  std::cout << "device authenticated" << std::endl;
              }
          }
          std::this_thread::sleep_for(100ms);
      }
  }};

  for (auto &&thread : threads) {
    thread.join();
  }
}
