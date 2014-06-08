#include <iostream>
#include <functional>

using std::cout;
using std::endl;

using nullfn = void (void);

template <typename Type, typename Result, typename ...Arguments>
struct property_method_traits {};

template <typename Type, typename Result, typename ...Arguments>
struct property_method_traits <Type, Result (Arguments...)>
{
  using method_type = std::function<Result (Type&, Arguments...)>;
  using result_type = typename method_type::result_type;
};

template <typename Type, typename AccessType, typename MutateType>
class basic_property
{
  public:
    using access_method = typename property_method_traits<Type, AccessType>::method_type;
    using access_result = typename property_method_traits<Type, AccessType>::result_type;

    using mutate_method = typename property_method_traits<Type, MutateType>::method_type;
    using mutate_result = typename property_method_traits<Type, MutateType>::result_type;

  public:
    template <typename Accessor, typename Mutator>
    property (Type init, Accessor accessor, Mutator mutator) :
      property_ {std::forward<Type> (init)},
      accessor_ {std::forward<Accessor> (accessor)},
      mutator_ {std::forward<Mutator> (mutator)} {}

  public:
    operator access_result ()
    {
      return accessor_ (property_);
    }

    mutate_result operator= (Type &&value)
    {
      return mutator_ (property_, std::forward<Type>(value));
    }

  private:
    Type property_;

  private:
    access_method accessor_;
    mutate_method mutator_;
};

template <typename Type, typename AccessType, typename MutateType>
class property
{
  public:
    using access_method = typename property_method_traits<Type, AccessType>::method_type;
    using access_result = typename property_method_traits<Type, AccessType>::result_type;

    using mutate_method = typename property_method_traits<Type, MutateType>::method_type;
    using mutate_result = typename property_method_traits<Type, MutateType>::result_type;

  public:
    template <typename Accessor, typename Mutator>
    property (Type init, Accessor accessor, Mutator mutator) :
      property_ {std::forward<Type> (init)},
      accessor_ {std::forward<Accessor> (accessor)},
      mutator_ {std::forward<Mutator> (mutator)} {}

  public:
    template <typename ...Args>
    auto get (Args ...args)
    {
      return accessor_ (property_, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    auto set (Args ...args)
    {
      return mutator_ (property_, std::forward<Args>(args)...);
    }

  private:
    Type property_;

  private:
    access_method accessor_;
    mutate_method mutator_;
};

int main()
{
  int init = 4;
  property<int&, int(void), void(int)> p
  {
    init,
    [](int &value) { return value + 1; },
    [](int &value, int v) { value = v / 2; }
  };

  p.set(8);
  cout << p.get() << endl;

  return 0;
}
