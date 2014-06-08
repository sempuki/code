#include <iostream>
#include <functional>

using std::cout;
using std::endl;

using nullfn_t = void (void);

template <typename Type, typename Result, typename ...Arguments>
struct property_method_traits {};

template <typename Type, typename Result, typename ...Arguments>
struct property_method_traits <Type, Result (Arguments...)>
{
  using method_type = std::function<Result (Type&, Arguments...)>;
  using result_type = typename method_type::result_type;
};

template <typename Type, typename AccessType, typename MutateType>
class property_base
{
  public:
    using property_type = Type;

    using access_method_type = typename property_method_traits<Type,AccessType>::method_type;
    using access_result_type = typename property_method_traits<Type,AccessType>::result_type;
    using mutate_method_type = typename property_method_traits<Type,MutateType>::method_type;
    using mutate_result_type = typename property_method_traits<Type,MutateType>::result_type;

  public:
    template <typename Accessor, typename Mutator>
    property_base (Type init, Accessor accessor, Mutator mutator) :
      property_ {std::forward<Type> (init)},
      accessor_ {std::forward<Accessor> (accessor)},
      mutator_ {std::forward<Mutator> (mutator)} {}

  protected:
    property_type property_;
    access_method_type accessor_;
    mutate_method_type mutator_;
};

template <typename Type, typename AccessType = nullfn_t, typename MutateType = nullfn_t>
class simple_property : 
  public property_base<Type, AccessType, MutateType>
{
  private:
    using base_type = property_base<Type,AccessType,MutateType>;
    using property_type = typename base_type::property_type;
    using access_result_type = typename base_type::access_result_type;
    using mutate_result_type = typename base_type::mutate_result_type;

  public:
    using base_type::base_type;

    operator access_result_type ()
    {
      return base_type::accessor_ (base_type::property_);
    }

    mutate_result_type operator= (property_type &&value)
    {
      return base_type::mutator_ (base_type::property_, std::forward<property_type>(value));
    }
};

template <typename Type, typename AccessType = nullfn_t, typename MutateType = nullfn_t>
class property :
  public property_base<Type, AccessType, MutateType>
{
  private:
    using base_type = property_base<Type,AccessType,MutateType>;

  public:
    using base_type::property_base;

  public:
    template <typename ...Args>
    auto get (Args ...args)
    {
      return base_type::accessor_ (base_type::property_, std::forward<Args>(args)...);
    }

    template <typename ...Args>
    auto set (Args ...args)
    {
      return base_type::mutator_ (base_type::property_, std::forward<Args>(args)...);
    }
};

int main()
{
  property<int, int(void), void(int)> p
  {
    10,
    [](int &value) { return value + 1; },
    [](int &value, int v) { value = v / 2; }
  };

  cout << p.get() << endl;
  p.set(5);
  cout << p.get() << endl;

  return 0;
}
