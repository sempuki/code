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
        private:
            class dispatch
            {
                public:
                    operator bool () const
                    {
                        return 
                            call_ != nullptr && 
                            copy_ != nullptr && 
                            destroy_ != nullptr;
                    }

                    bool operator== (dispatch const &other) const
                    {
                        return 
                            call_ != other.call_ && 
                            copy_ != other.copy_ && 
                            destroy_ != other.destroy_;
                    }

                    inline void call (void *storage, Args... args)
                    {
                        (this->*call_) (storage, args...);
                    }

                    inline void copy (void const *storage, void *destination)
                    {
                        (this->*copy_) (storage, destination);
                    }

                    inline void destroy (void *storage)
                    {
                        (this->*destroy_) (storage);
                    }

                    void clear ()
                    {
                        call_ = nullptr; 
                        copy_ = nullptr; 
                        destroy_ = nullptr;
                    }

                    template <typename Functor>
                    void initialize ()
                    {
                        call_ = &dispatch::functor_call<Functor>;
                        copy_ = &dispatch::functor_copy<Functor>;
                        destroy_ = &dispatch::functor_destroy<Functor>;
                    }

                private:
                    template <typename Functor>
                    void functor_call (void *storage, Args... args)
                    {
                        auto typed_storage = reinterpret_cast<Functor *> (storage);
                        (*typed_storage) (args...);
                    }

                    template <typename Functor>
                    void functor_copy (void const *storage, void *destination)
                    {
                        auto typed_storage = reinterpret_cast<Functor const *> (storage);
                        new (destination) Functor (*typed_storage);
                    }

                    template <typename Functor>
                    void functor_destroy (void *storage)
                    {
                        auto typed_storage = reinterpret_cast<Functor *> (storage);
                        typed_storage->~Functor();
                    }

                private:
                    void (dispatch::*call_) (void *storage, Args... args) = nullptr;
                    void (dispatch::*copy_) (void const *storage, void *dst) = nullptr;
                    void (dispatch::*destroy_) (void *storage) = nullptr;
            };

            // Tune memory requirements for your application or platform
            using storage = std::aligned_storage<32, 8>::type; 

        public:

            delegate () = default;

            delegate (delegate const &other) :
                dispatch_ {other.dispatch_}
            {
                dispatch_.copy (&other.storage_, &storage_);
            }

            template <typename Functor>
            delegate (Functor functor)
            {
                assert (sizeof (functor) <= sizeof (&storage_));
                dispatch_.template initialize<Functor> ();
                dispatch_.copy (&functor, &storage_);
            }

            ~delegate ()
            {
                dispatch_.destroy (&storage_);
            }

        public:
            delegate &operator= (std::nullptr_t)
            {
                dispatch_.clear ();
                return *this;
            }

            delegate &operator= (delegate const &other)
            {
                dispatch_.copy (&other.storage_, &storage_);
                return *this;
            }

            template <typename Functor>
            delegate &operator= (Functor functor)
            {
                assert (sizeof (functor) <= sizeof (&storage_));
                dispatch_.template initialize<Functor> ();
                dispatch_.copy (&functor, &storage_);
                return *this;
            }

        public:
            operator bool () const 
            { 
                return dispatch_;
            }

            bool operator== (delegate const &other) const
            {
                return dispatch_ == other.dispatch_;
            }

            void operator() (Args... args)
            {
                dispatch_.call (&storage_, args...);
            }

        private:
            dispatch dispatch_;
            storage  storage_;
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
