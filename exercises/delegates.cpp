#include <iostream>
#include <functional>
#include <vector>
#include <cstring>
#include <cassert>

using namespace std;

struct Test
{
    void foo (int value)
    {
        cout << "Test::foo (" << value << ") {" << count++ << "}" << endl;
    }

    int count = 0;
};

namespace core
{
    template <typename ...Args>
    class delegate
    {
        static const int STORAGE_SIZE = 64 - sizeof (void (delegate::*)(Args...));

        public:
            delegate () : 
                invoker_ {nullptr} 
            {}

            delegate (delegate const &copy) :
                invoker_ {copy.invoker_}
            {
                std::memcpy (storage_, copy.storage_, STORAGE_SIZE);
            }

            delegate (void (*function)(Args...))
            {
                assign (function);
            }

            template <typename Functor>
            delegate (Functor functor)
            {
                assign (functor);
            }

        public:
            operator bool () const 
            { 
                return invoker_ != nullptr; 
            }

            bool operator== (delegate const &rhs) const
            {
                return 
                    invoker_ != nullptr &&
                    invoker_ == rhs.invoker_ && 
                    std::memcmp (storage_, rhs.storage_, STORAGE_SIZE) == 0;
            }

            delegate &operator= (std::nullptr_t)
            {
                clear ();
                return *this;
            }

            delegate &operator= (void (*function)(Args...))
            {
                assign (function);
                return *this;
            }

            template <typename Functor>
            delegate &operator= (Functor &&functor)
            {
                assign (functor);
                return *this;
            }

            void operator() (Args... args)
            {
                (this->*invoker_) (args...);
            }

        private:
            void clear ()
            {
                std::memset (storage_, 0, STORAGE_SIZE);
                invoker_ = nullptr;
            }

            void assign (void (*function)(Args...))
            {
                assert (sizeof (function) <= STORAGE_SIZE);
                std::memset (storage_, 0, STORAGE_SIZE);
                std::memcpy (storage_, &function, sizeof (function));
                invoker_ = &delegate::invoke;
            }

            template <typename Functor>
            void assign (Functor functor)
            {
                assert (sizeof (functor) <= STORAGE_SIZE);
                std::memset (storage_, 0, STORAGE_SIZE);
                std::memcpy (storage_, &functor, sizeof (functor));
                invoker_ = &delegate::invoke<Functor>;
            }

            void invoke (Args... args)
            {
                (**reinterpret_cast <void (**)(Args...)> (storage_)) (args...);
            }

            template <typename Functor>
            void invoke (Args... args)
            {
                (*reinterpret_cast<Functor *> (storage_)) (args...);
            }

        private:
            void (delegate::*invoker_) (Args... args);
            uint8_t storage_ [STORAGE_SIZE];
    };

    template <typename ...Args>
    class signal
    {
        public:
            class slot
            {
                friend class signal;

                public:
                    operator bool () const 
                    { 
                        return signal_ != nullptr && index_ != -1; 
                    }

                private:
                    void attach (signal const *p, int i)
                    {
                        signal_ = p;
                        index_ = i;
                    }

                    void detach ()
                    {
                        signal_ = nullptr;
                        index_ = -1;
                    }

                    bool belongs (signal const *p, int n) const
                    {
                        return signal_ == p && index_ < n;
                    }

                    int index () const
                    {
                        return index_;
                    }

                private:
                    signal const *signal_ = nullptr;
                    int index_ = -1;
            };

        public:
            template <typename Functor>
            slot operator+= (Functor functor)
            {
                slot incoming;

                int size = delegates.size();
                for (int index = 0; index < size; ++index)
                {
                    if (!delegates[index])
                    {
                        delegates[index] = functor;
                        incoming.attach (this, index);
                        break;
                    }
                }

                if (!incoming)
                {
                    delegates.emplace_back (functor);
                    incoming.attach (this, size);
                }

                return incoming;
            }

            void operator-= (slot &outgoing)
            {
                if (outgoing.belongs (this, delegates.size()))
                {
                    int index = outgoing.index();
                    delegates[index] = nullptr;
                    outgoing.detach();
                }
            }

            void operator() (Args... args)
            {
                for (auto &next : delegates)
                    if (next) next (args...);
            }

        private:
            std::vector<delegate<Args...>> delegates;
    };
}

void dummy (int value)
{
    cout << "dummy (" << value << ")" << endl;
}

int main()
{
    Test test;

    core::signal<int> signal;
    core::signal<int>::slot last;

    for (int i=0; i < 3; ++i)
    {
        last = (signal += [&test] (int value) { test.foo (value); });
    }

    signal (5);
    signal (6);
    signal (7);

    return 0;
}
