#include <iostream>
#include <functional>

using std::cout;
using std::endl;

template <typename Type>
class property
{
  public:
    template <typename Getter, typename Setter>
    property (Getter &&getter, Setter &&setter) :
      getter_ {std::forward<Getter> (getter)},
      setter_ {std::forward<Setter> (setter)} {}

    template <typename Getter, typename Setter>
    property (Type init, Getter &&getter, Setter &&setter) :
      property_ {std::move (init)},
      getter_ {std::forward<Getter> (getter)},
      setter_ {std::forward<Setter> (setter)} {}

    property (property const &that) :
      property_ {that.property_},
      getter_ {that.getter_}, 
      setter_ {that.setter_} {}

    property (property &&that) :
      property_ {std::move (that.property_)},
      getter_ {std::move (that.getter_)}, 
      setter_ {std::move (that.setter_)} {}

    property &operator= (property that)
    {
      property_ = std::move (that.property_);
      setter_ = std::move (that.setter_);
      getter_ = std::move (that.getter_);
      return *this;
    }

    Type operator() () { return getter_(property_); }
    const Type operator() () const { return getter_(property_); }

    operator Type () { return getter_(property_); }
    operator const Type () const { return getter_(property_); }

    property &operator= (Type const &value)
    {
      property_ = setter_ (value);
      return *this;
    }

  private:
    Type property_;
    std::function<Type (Type const &)> getter_;
    std::function<Type (Type const &)> setter_;
};
    
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

struct Test
{
  Test () {}
  property<Object> prop
  {
    [](Object const &p) { std::cout << "got object" << std::endl; return p; },
    [](Object const &v) { std::cout << "set object" << std::endl; return v; }
  };
};

int main()
{
  Test t;
  std::cout << t.prop().v << std::endl;

  return 0;
}
