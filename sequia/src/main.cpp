#include <iostream>
#include <iomanip>

#include <core/debug.hpp>
#include <core/standard.hpp>
#include <memory/core.hpp>
#include <core/types.hpp>
#include <core/bits.hpp>
#include <core/hash.hpp>
#include <core/name.hpp>
#include <core/container.hpp>
#include <core/stream.hpp>
#include <state/state.hpp>
#include <memory/layout.hpp>

// TODO: per-namespace meta-include file

using namespace std;
using namespace sequia;

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
    const int N = 10;
    int memory[N];
    core::stream<int> s1 {memory::buffer<int> {memory, N}};

    for (int i=0; i < 5; ++i)
        s1 << i;

    for (int i=0; i < 5; ++i)
        s1 >> i, cout << i << ",";
    cout << endl;

    for (int i=0; i < 5; ++i)
        s1 << i;
    cout << "stream size: " << s1.size() << endl;


    state::singular_machine <State1, State2, State3> machine;
    machine.react (1);
    machine.react (2);
    machine.react (3);
    machine.react (4);
    machine.react (5);
    machine.react (6);

    core::static_vector<int, 10> v1;

    for (int i=0; i < 10; ++i)
        v1.push_back (i);

    for (int i=0; i < 10; i++)
        cout << v1[i] << ",";
    cout << endl;

    auto v2 = core::make_fixed_vector<int> (10);
    core::fixed_vector<int> v3 = v2;

    for (int i=0; i < 10; ++i)
        v3.push_back (i);

    for (int i=0; i < 10; i++)
        cout << v3[i] << ",";
    cout << endl;

    core::static_map<int, int, 10> m1;

    for (int i=0; i < 10; i++)
        m1[i] = i;

    for (int i=0; i < 10; i++)
        cout << m1[i] << ",";
    cout << endl;

    auto m2 = core::make_fixed_map<int,int> (10);

    for (int i=0; i < 10; ++i)
        m2[i] = i;

    for (int i=0; i < 10; i++)
        cout << m2[i] << ",";
    cout << endl;

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
    cout << "0x" << std::setw(8) << std::setfill('0') << n0 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n1 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n2 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n3 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n4 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n5 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n6 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n7 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n8 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << n9 << endl;

    return 0; 
}
