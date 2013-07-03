#include <iostream>
#include <iomanip>

#include <core/debug.hpp>
#include <core/standard.hpp>
#include <core/types.hpp>
#include <core/bits.hpp>
#include <memory/core.hpp>
#include <core/hash.hpp>
#include <core/name.hpp>
#include <core/container.hpp>
#include <state/state.hpp>

#include <memory/layout.hpp>
#include <io/file/chunk.hpp>
#include <io/file/format.hpp>
#include <data/endian.hpp>
#include <data/encoding/bit.hpp>
#include <data/map.hpp>
#include <data/file/heap_description.hpp>

#include <policy/data/mapper.hpp>
#include <core/stream.hpp>

#include <system/platform.hpp>
#include <io/net/socket.hpp>

// TODO: per-namespace meta-include file

using namespace std;
using namespace sequia;
using namespace sequia::data::map;

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

int main (int argc, char **argv)
{
    size_t size = 16;
    uint8_t memory[size];

    std::fill_n (memory, size, 0);
    core::bytestream stream {{memory, size}};

    stream << (int32_t) 0xABCDDCBA;
    stream << (int32_t) 0xFF00FF00;
    stream << (int32_t) 0x00FF00FF;
    stream << (int32_t) 0xAABBAABB;

    uint8_t i = 0;
    while (!stream.empty ())
        stream >> i, cout << std::hex << (int) i << endl;

    stream.reset ();
    for (uint8_t i = 0; i < size; ++i)
        stream << i;

    std::fstream file ("test.txt", std::ios_base::in);

    if (file)
    {
        data::file::heap_description descr;

        file >> descr;

        cout << descr.name << " " << descr.page_size << " " 
            << descr.min_pages << " " << descr.max_pages << endl;

        file.close();
    }

    state::singular_machine <State1, State2, State3> machine;
    machine.react (1);
    machine.react (2);
    machine.react (3);
    machine.react (4);
    machine.react (5);
    machine.react (6);

    core::static_vector<bool, 10> v1;

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
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n0 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n1 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n2 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n3 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n4 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n5 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n6 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n7 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n8 << endl;
    cout << "0x" << std::setw(8) << std::setfill('0') << (uint32_t) n9 << endl;

    io::net::socket::address addr {"127.0.0.1:8080"};
    cout << (std::string) addr << endl;
    cout << addr.host() << endl;
    cout << addr.port() << endl;

    io::net::socket sock {io::net::socket::type::UDP};

    //scoped_thread network { std::thread { [] { } } };

    //bool quit = false;
    //std::string command;

    //while (!quit && cin >> command)
    //{
    //    if (command == "quit")
    //        quit = true;
    //}

    return 0; 
}
