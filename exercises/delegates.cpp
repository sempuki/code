#include <iostream>
#include <functional>
#include <vector>
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

template <typename ...Args>
class Delegate
{
    static const int STORAGE_SIZE = 64 - sizeof (void (Delegate::*)(Args...));

    public:
        Delegate () : 
            invoker_ {nullptr} 
        {}

        Delegate (Delegate const &copy) :
            invoker_ {copy.invoker_}
        {
            std::memcpy (storage_, copy.storage_, STORAGE_SIZE);
        }
        
        Delegate (void (*function)(Args...))
        {
            assign (function);
        }

        template <typename Functor>
        Delegate (Functor functor)
        {
            assign (functor);
        }

    public:
        operator bool () const 
        { 
            return invoker_ != nullptr; 
        }

        bool operator== (Delegate const &rhs) const
        {
            return 
                invoker_ != nullptr &&
                invoker_ == rhs.invoker_ && 
                std::memcmp (storage_, rhs.storage_, STORAGE_SIZE) == 0;
        }

        Delegate &operator= (void (*function)(Args...))
        {
            assign (function);
            return *this;
        }

        template <typename Functor>
        Delegate &operator= (Functor &&functor)
        {
            assign (functor);
            return *this;
        }

        void operator() (Args... args)
        {
            (this->*invoker_) (args...);
        }

    private:
        template <typename Functor>
        void assign (Functor functor)
        {
            assert (sizeof (functor) <= STORAGE_SIZE);
            std::memset (storage_, 0, STORAGE_SIZE);
            std::memcpy (storage_, &functor, sizeof (functor));
            invoker_ = &Delegate::invoke<Functor>;
        }

        void assign (void (*function)(Args...))
        {
            assert (sizeof (function) <= STORAGE_SIZE);
            std::memset (storage_, 0, STORAGE_SIZE);
            std::memcpy (storage_, &function, sizeof (function));
            invoker_ = &Delegate::invoke;
        }

        template <typename Functor>
        void invoke (Args... args)
        {
            (*reinterpret_cast<Functor *> (storage_)) (args...);
        }

        void invoke (Args... args)
        {
            auto test = reinterpret_cast <void (*)(Args...)> (storage_);
            cout << (void *) test << endl;
        }

    private:
        void (Delegate::*invoker_) (Args... args);
        uint8_t storage_ [STORAGE_SIZE];
};

template <typename ...Args>
class Signal
{
    public:
        template <typename Functor>
        void operator+= (Functor functor)
        {
            bool found = false;

            for (auto &delegate : delegates)
            {
                if (!delegate)
                {
                    delegate = functor;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                delegates.emplace_back (functor);
            }
        }

        template <typename Functor>
        void operator-= (Functor functor)
        {
        }

        void operator() (Args... args)
        {
            for (auto &delegate : delegates)
                if (delegate)
                    delegate (args...);
        }

    private:
        std::vector<Delegate<Args...>> delegates;
};

void dummy (int value)
{
    cout << "dummy (" << value << ")" << endl;
}

int main()
{
    Test test;

    cout << (void *) dummy << endl;

    Delegate<int> delegates[10];

    for (auto &delegate : delegates)
    {
        delegate = dummy;//[test] (int value) { test.foo (value); };
    }

    int value = 0;
    for (auto &delegate : delegates)
    {
        delegate (value++);
    }

    return 0;
}
