#include <iostream>
#include <functional>

using std::cout;
using std::endl;

template <typename Type>
class property
{
  public:
    using type = Type;

  public:
    template <typename Get, typename Set>
    property (Get &&get, Set &&set) :
      get_ {std::forward<Get> (get)},
      set_ {std::forward<Set> (set)} {}

    template <typename Get, typename Set>
    property (Type init, Get &&get, Set &&set) :
      property_ {std::move (init)},
      get_ {std::forward<Get> (get)},
      set_ {std::forward<Set> (set)} {}

    property (property const &that) :
      property_ {that.property_},
      get_ {that.get_}, 
      set_ {that.set_} {}

    property (property &&that) :
      property_ {std::move (that.property_)},
      get_ {std::move (that.get_)}, 
      set_ {std::move (that.set_)} {}

    property &operator= (property that)
    {
      property_ = std::move (that.property_);
      set_ = std::move (that.set_);
      get_ = std::move (that.get_);
      return *this;
    }

    operator Type &() { return (*this)(); }
    operator const Type &() const { return (*this)(); }

    Type &operator() () 
    { 
      get_ (property_); 
      return property_; 
    }

    const Type &operator() () const 
    { 
      get_ (property_); 
      return property_; 
    }

    property &operator= (Type value)
    {
      set_ (property_, std::move (value));
      return *this;
    }

  private:
    template<typename T> friend std::ostream &operator<<(std::ostream &, property<T> const &);
    template<typename T> friend std::ostream &operator>>(std::ostream &, property<T> &);

  private:
    Type property_;
    std::function<void (Type const &)> get_;
    std::function<void (Type &, Type)> set_;
};

template<typename T>
std::ostream &operator<<(std::ostream &output, property<T> const &p)
{
  return output << p.property_;
}

template<typename T>
std::istream &operator>>(std::istream &input, property<T> &p)
{
  return input >> p.property_;
}

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
  Test () {}
  property<Object> prop
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

int main()
{
  Test t;
  Object v {6}, r;

  t.prop = v;
  std::cout << t.prop << std::endl;
  r = t.prop;
  std::cout << r.v << std::endl;

  return 0;
}
