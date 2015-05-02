#ifndef SRC_CORE_HANDLE_HPP_
#define SRC_CORE_HANDLE_HPP_

namespace core {

template <typename Type = uint32_t>
class Handle {
 public:
  Handle() : value_ {} {}
  explicit Handle(Type value) : value_ {value} {}

 public:
  Handle &operator=(const Handle &that) {
    value_ = that.value_;
    return *this;
  }

 public:
  Type value() const { return value_; }
  explicit operator bool() const { return value_; }

 public:
  bool operator==(const Handle &that) const { return value_ == that.value_; }
  bool operator!=(const Handle &that) const { return value_ != that.value_; }
  bool operator< (const Handle &that) const { return value_ <  that.value_; }
  bool operator<=(const Handle &that) const { return value_ <= that.value_; }
  bool operator> (const Handle &that) const { return value_ >  that.value_; }
  bool operator>=(const Handle &that) const { return value_ >= that.value_; }

 public:
  template<typename Value>
  static Handle<Type> FromHash(const Value &value) {
    return Handle<Type> {
      static_cast<Type>(std::hash<Value>{}(value))  // NOLINT
    };
  }

 private:
  Type value_;
};

}  // namespace core

#endif
