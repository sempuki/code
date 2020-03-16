#pragma once

#include <atomic>
#include <experimental/source_location>
#include <map>
#include <string_view>

namespace std {
using experimental::source_location;
}

namespace ctl {

struct code {
  int32_t kind;
  int32_t item;
};

class wall;

class enclosure {
 public:
  virtual bool equivalent(code this_code, code that_code,
                          enclosure const& that_enclosure) = 0;

  virtual std::string const& message(wall const& wall) = 0;
  virtual std::source_location const& location(wall const& wall) = 0;

 protected:
  wall create(code code);
  code reveal(wall wall);
};

class wall final {
 public:
  wall() = default;
  ~wall() = default;

  wall(wall const&) = default;
  wall& operator=(wall const&) = default;

  explicit operator bool() const noexcept { enclosure_ != nullptr; }

  bool operator==(wall const& that) const noexcept {
    return (enclosure_ == that.enclosure_ && code_.kind == that.code_.kind);
  }

  std::string const& message() const noexcept {
    return enclosure_->message(*this);
  }

  std::source_location const& location() const noexcept {
    return enclosure_->location(*this);
  }

 private:
  friend enclosure;

  wall(enclosure* enclosure, code code) : enclosure_{enclosure}, code_{code} {}

  enclosure* enclosure_ = nullptr;
  code code_;
};

struct kind_entry {
  // ~64 characters?
  std::string message;
};

struct item_entry {
  std::source_location location;
  std::string extra;
};

//
class posix_enclosure final : public enclosure {
 public:
  wall raise(int error, std::source_location location,
             std::string_view extra = {}) {
    wall wall = create({
        static_cast<int32_t>(error),
        static_cast<int32_t>(next_),
    });

    items_[next_].location = location;
    items_[next_].extra = extra;
    next_ = (next_ + 1) % items_.size();

    return wall;
  }

  virtual bool equivalent(code this_code, code that_code,
                          enclosure const& that_enclosure) {
    return false;
  }

  std::string const& message(wall const& wall) override {
    auto [kind, _] = enclosure::reveal(wall);
    return kinds_[kind].message;
  }

  std::source_location const& location(wall const& wall) override {
    auto [_, item] = enclosure::reveal(wall);
    return items_[item].location;
  }

 private:
  static std::array<kind_entry, 125> const kinds_;

  std::array<item_entry, 128> items_;
  std::size_t next_ = 0;
};

class win32_enclosure final : public enclosure {
 public:
  wall raise(int error, std::source_location location,
             std::string_view extra = {}) {
    wall wall = create({
        static_cast<int32_t>(error),
        static_cast<int32_t>(next_),
    });

    items_[next_].location = location;
    items_[next_].extra = extra;
    next_ = (next_ + 1) % items_.size();

    return wall;
  }

  virtual bool equivalent(code this_code, code that_code,
                          enclosure const& that_enclosure) {
    return false;
  }

  std::string const& message(wall const& wall) override {
    auto [kind, _] = enclosure::reveal(wall);
    return kinds_.at(kind).message;
  }

  std::source_location const& location(wall const& wall) override {
    auto [_, item] = enclosure::reveal(wall);
    return items_[item].location;
  }

 private:
  static std::map<int, kind_entry> const kinds_;

  std::array<item_entry, 128> items_;
  std::size_t next_ = 0;
};

}  // namespace ctl
