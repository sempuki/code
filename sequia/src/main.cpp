#include <iostream>
#include <iomanip>

#include <core/debug.hpp>
#include <core/standard.hpp>
#include <core/stream.hpp>
#include <core/types.hpp>
#include <core/name.hpp>
#include <core/container.hpp>
#include <state/state.hpp>

using namespace std;
using namespace sequia;

namespace App
{
    class Test
    {
        public:
            Test(int a, float b, char c)
                : a_(a), b_(b), c_(c) {}

            template <typename T>
            bool serialize (core::stream<T> *s) const
            {
                *s << a_ << b_ << c_;
                return true;
            }

            template <typename T>
            bool deserialize (core::stream<T> *s)
            {
                *s >> a_ >> b_ >> c_;
                return true;
            }

        private:
            int     a_;
            float   b_;
            char    c_;
    };
}

namespace traits
{
    namespace stream
    {
        template <>
        struct element <App::Test>
        {
            typedef custom_serializable_tag serialization;
        };
    }
}

struct State1
{
    State1 () { on_default (); }
    State1 (int i) { on_enter (i); }
    ~State1 () { on_exit (); }

    virtual void on_default () 
    { 
        cout << "entered " << typeid(State1).name() << " as default " << endl; 
    }

    virtual void on_enter (int i) 
    { 
        cout << "entered " << typeid(State1).name() << " on event " << i << endl; 
    }

    virtual void on_exit () 
    { 
        cout << "exited " << typeid(State1).name() << endl; 
    }
};

struct State2
{
    State2 () { on_default (); }
    State2 (int i) { on_enter (i); }
    ~State2 () { on_exit (); }

    virtual void on_default () 
    { 
        cout << "entered " << typeid(State2).name() << " as default " << endl; 
    }

    virtual void on_enter (int i) 
    { 
        cout << "entered " << typeid(State2).name() << " on event " << i << endl; 
    }

    virtual void on_exit () 
    { 
        cout << "exited " << typeid(State2).name() << endl; 
    }
};

struct State3
{
    State3 () { on_default (); }
    State3 (int i) { on_enter (i); }
    ~State3 () { on_exit (); }

    virtual void on_default () 
    { 
        cout << "entered " << typeid(State3).name() << " as default " << endl; 
    }

    virtual void on_enter (int i) 
    { 
        cout << "entered " << typeid(State3).name() << " on event " << i << endl; 
    }

    virtual void on_exit () 
    { 
        cout << "exited " << typeid(State3).name() << endl; 
    }
};

namespace traits
{
    namespace state
    {
        // use argument depended lookup?
        template <> struct transition <State1, int> { typedef State2 next; };
        template <> struct transition <State2, int> { typedef State3 next; };
        template <> struct transition <State3, int> { typedef State1 next; };
    }
}

int main(int argc, char **argv)
{
    size_t N = sizeof(App::Test) * 10;
    uint8_t buf[N];

    core::stream<uint8_t> stream (buf, N);
    App::Test in (1, 0.1, 'a');
    App::Test out (0, 0.0, 0);
    
    stream << in;
    stream >> out;

    state::singular_machine <State1, State2, State3> machine;
    machine.react (1);
    machine.react (2);
    machine.react (3);
    machine.react (4);
    machine.react (5);
    machine.react (6);

    core::fixed_vector<int, 10> vec;

    for (int i=0; i < 10; ++i)
        vec.push_back (i);

    for (int i=0; i < 10; ++i)
        cout << vec[i] << endl;

    core::fixed_map<10, int, int> map;

    for (int i=0; i < 10; i++)
        map[i] = i;

    for (int i=0; i < 10; i++)
        cout << map[i] << endl;

    core::name n0 ("521");
    core::name n1 ("01293e121.,m.1e,23332l4k342oi7ccf");
    core::name n2 ("dfasdlkfjasldk");
    core::name n3 ("lkdjgalskdjflajdslkf");
    core::name n4 ("kasdjf");
    core::name n5 ("jadl;fja;slk");
    core::name n6 ("aldskfjalsdjfalksj");
    core::name n7 ("dsfsaj");
    core::name n8 ("sadlfasldkfjals;dkjf;lasdkj");
    core::name n9 ("sdfljlasdicuvxc.,merw.,e");

    cout << std::hex;
    cout << "0x" << std::setw(8) << std::setfill('0') << n0.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n1.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n2.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n3.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n4.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n5.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n6.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n7.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n8.digest_ << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n9.digest_ << endl;

    return 0; 
}
