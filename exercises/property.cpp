#include <iostream>
#include <functional>
#include <type_traits>
#include <vector>

using std::cout;
using std::endl;

namespace util {

template <typename ...Args>
class signal
{
  public:
    signal &operator=(std::nullptr_t)
    {
      delegates_.clear();
      return *this;
    }

    signal &operator=(signal that)
    {
      delegates_ = std::move(that.delegates_);
      return *this;
    }

    signal &operator+=(signal that)
    {
      delegates_.insert(std::end(delegates_),
          std::make_move_iterator(std::begin(that.delegates_)),
          std::make_move_iterator(std::end(that.delegates_)));
      return *this;
    }

    template <typename Functional>
    signal &operator+=(Functional callback)
    {
      delegates_.emplace_back(std::move(callback));
      return *this;
    }

    void operator()(Args ...args)
    {
      for (auto &delegate : delegates_)
        delegate(args...);
    }

  private:
    std::vector<std::function<void(Args...)>> delegates_;
};


template <typename Type>
class property_base
{
  public:
    property_base () : 
      value_ {Type()} {}

    property_base (Type init) : 
      value_ {std::move (init)} {}
    
    property_base (property_base const &that) :
      value_ {that.value_} {}

    property_base (property_base &&that) :
      value_ {std::move (that.value_)} {}

    property_base operator= (property_base that)
    {
      value_ = std::move(that.value_);
    }

  public:
    Type &value() { return value_; }
    Type const &value() const { return value_; }

  public:
    virtual void on_change() {}
    void change (Type value) 
    { 
      value_ = std::move(value); 
      on_change();
    }

  private:
    Type value_;
};

template <typename Type>
class property_readonly : 
  protected property_base<Type>
{
  public:
    template <typename Get>
    property_readonly (Get &&get) :
      getter_ {std::forward<Get> (get)} {}

    template <typename Get>
    property_readonly (Type init, Get &&get) :
      property_base<Type> {std::move (init)},
      getter_ {std::forward<Get> (get)} {}

    property_readonly (property_readonly const &that) :
      property_base<Type> {that},
      getter_ {that.getter_} {} 

    property_readonly (property_readonly &&that) :
      property_base<Type> {std::move (that)},
      getter_ {std::move (that.getter_)} {} 

  public:
    Type &operator() () 
    { 
      getter_ (this->value()); 
      return this->value(); 
    }

    const Type &operator() () const 
    { 
      getter_ (this->value()); 
      return this->value(); 
    }

    operator Type &() { return (*this)(); }
    operator const Type &() const { return (*this)(); }

  private:
    std::function<void (Type const &)> getter_;
};


template <typename Type>
class property_readwrite :
  protected property_base<Type>
{
  public:
    template <typename Get, typename Set>
    property_readwrite (Get &&get, Set &&set) :
      getter_ {std::forward<Get> (get)},
      setter_ {std::forward<Set> (set)} {}

    template <typename Get, typename Set>
    property_readwrite (Type init, Get &&get, Set &&set) :
      property_base<Type> {std::move (init)},
      getter_ {std::forward<Get> (get)},
      setter_ {std::forward<Set> (set)} {}

    property_readwrite (property_readwrite const &that) :
      property_base<Type> {that},
      getter_ {that.getter_}, 
      setter_ {that.setter_} {}

    property_readwrite (property_readwrite &&that) :
      property_base<Type> {std::move (that)},
      getter_ {std::move (that.getter_)}, 
      setter_ {std::move (that.setter_)} {}

    property_readwrite &operator= (property_readwrite that)
    {
      property_base<Type>::operator= (std::move (that));
      setter_ = std::move (that.setter_);
      getter_ = std::move (that.getter_);
      return *this;
    }

  public:
    Type &operator() () 
    { 
      getter_ (this->value()); 
      return this->value(); 
    }

    const Type &operator() () const 
    { 
      getter_ (this->value()); 
      return this->value(); 
    }
    
    operator Type &() { return (*this)(); }
    operator const Type &() const { return (*this)(); }

    property_readwrite &operator= (Type v)
    {
      setter_ (this->value(), std::move (v));
      this->on_change();
      return *this;
    }

  private:
    std::function<void (Type const &)> getter_;
    std::function<void (Type &, Type)> setter_;
};

struct readonly {};
struct readwrite {};
struct none {};

template <bool B, typename Then, typename Else> struct if_else {};
template <typename Then, typename Else> struct if_else <true, Then, Else> { using type = Then; };
template <typename Then, typename Else> struct if_else <false, Then, Else> { using type = Else; };

template <typename Type, typename Kind = readwrite, typename Friend = none>
class property : 
  public if_else<std::is_same<Kind, readonly>::value, 
    property_readonly<Type>, 
    property_readwrite<Type>>::type
{
  public:
    using type = Type;
    using kind = Kind;
    using base = typename if_else<std::is_same<Kind, readonly>::value, 
            property_readonly<Type>, 
            property_readwrite<Type>>::type;

    using base::base;
    using base::operator=;

  public:
    signal<Type const &> change;
  
  private:
    void on_change() { change(*this); }

  private:
    friend Friend;
    template<typename T> friend std::ostream &operator<<(std::ostream &output, property<T> const &p);
    template<typename T> friend std::istream &operator>>(std::istream &input, property<T> &p);
};

template<typename T>
std::ostream &operator<<(std::ostream &output, property<T> const &p)
{
  return output << p.value();
}

template<typename T>
std::istream &operator>>(std::istream &input, property<T> &p)
{
  return input >> p.value();
}

} // util::


struct Object
{
  Object() { std::cout << "default object" << std::endl; }
  Object(int i) : v {i} { std::cout << "init object" << std::endl; }
  Object(Object &&that) : v {that.v} { std::cout << "move object" << std::endl; }
  Object(Object const &that) : v {that.v} { std::cout << "copy object" << std::endl; }
  void operator=(Object &&that) { v = that.v; std::cout << "move assign object" << std::endl; }
  void operator=(Object const &that) { v = that.v; std::cout << "copy assign object" << std::endl; }
  ~Object() { std::cout << "destroy object" << std::endl; }
  
  int v = 5;
};

std::ostream &operator<<(std::ostream &output, Object const &obj)
{
  return output << obj.v;
}

struct Test
{
  Test () 
  {
    prop.change += [] cout << "hello world" << endl;
  }

  util::property<Object> prop
  {
    [](Object const &p) 
    { 
      std::cout << "got object" << std::endl; 
    },
    [](Object &p, Object v) 
    { 
      std::cout << "set object" << std::endl; 
      p = std::move(v); 
    }
  };
};

struct Foo 
{
  void method()
  {
    std::cout << "wtf" << std::endl;
  }
};

int main()
{
  Test t;
  Object v {6}, r;

  t.prop = v;
  std::cout << t.prop << std::endl;
  r = t.prop;
  std::cout << r.v << std::endl;

  Foo f;
  f.method();

  return 0;
}
