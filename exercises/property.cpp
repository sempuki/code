#include <iostream>
#include <functional>

using std::cout;
using std::endl;

template <typename GetType, typename SetType = GetType>
class property
{
  public:
    template <typename Accessor, typename Mutator>
    property (Accessor accessor, Mutator mutator) :
      accessor_ {std::forward<Accessor> (accessor)},
      mutator_ {std::forward<Mutator> (mutator)} {}

    template <typename Type>
    operator Type () { return get(); }

    template <typename Type>
    property &operator= (Type &&value)
    {
      set (std::forward<Type> (value));
      return *this;
    }

    GetType get() { return accessor_(); }
    const GetType get() const { return accessor_(); }
    void set(SetType value) { mutator_ (value); }

  private:
    std::function<GetType (void)> accessor_;
    std::function<void (SetType)> mutator_;
};
    
struct Object
{
  Object() { std::cout << "default object" << std::endl; }
  Object(int i) : v {i} { std::cout << "init object" << std::endl; }
  Object(Object &&) { std::cout << "move object" << std::endl; }
  Object(Object const &) { std::cout << "copy object" << std::endl; }
  void operator=(Object &&) { std::cout << "move assign object" << std::endl; }
  void operator=(Object const &) { std::cout << "copy assign object" << std::endl; }
  ~Object() { std::cout << "destroy object" << std::endl; }
  
  int v = 5;
};

class Test
{
  private:
    Object obj {4};

  public:
    property<Object &, Object const &> prop
    {
      [&]() -> Object& { std::cout << "got object" << std::endl; return obj; },
      [&](Object const &v) { std::cout << "set object" << std::endl; obj = v; }
    };
};

int main()
{
  Test t;
  Object v = { 6 };

  std::cout << "-----" << std::endl;
  std::cout << ": " << t.prop.get().v << std::endl;
  t.prop = v;
  std::cout << ": " << t.prop.get().v << std::endl;
  v = t.prop;
  std::cout << ": " << t.prop.get().v << std::endl;
  std::cout << "-----" << std::endl;

  return 0;
}
