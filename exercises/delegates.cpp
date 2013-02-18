#include <iostream>
#include <functional>
#include <cstring>
#include <cassert>

using namespace std;

struct Test
{
    void foo (int value) const
    {
        cout << "Test::foo (" << value << ")" << endl;
    }
};

struct Delegate
{
    static const int STORAGE_SIZE = 64;

    template <typename Functor>
    void call (int arg)
    {
        auto functor = reinterpret_cast <Functor *> (storage);
        (*functor) (arg);
    }

    template <typename Functor>
    void operator+= (Functor functor)
    {
        assert (sizeof (functor) <= STORAGE_SIZE);
        auto bytes = reinterpret_cast <uint8_t *> (&functor);
        std::memcpy (storage, bytes, sizeof (functor));
        caller = (&Delegate::call<Functor>);
    }

    void operator() (int arg)
    {
        (this->*caller) (arg);
    }

    uint8_t storage [STORAGE_SIZE];
    void (Delegate::*caller) (int);
};

int main()
{
    Test test;
    //auto method = &Test::foo;
    //auto t = ref(method);
    //t(test);

    Delegate delegates[10];

    for (auto &delegate : delegates)
    {
        delegate += [&test] (int i) { test.foo (i); };
    }

    int i = 0;
    for (auto &delegate : delegates)
    {
        delegate (i++);
    }

    return 0;
}
