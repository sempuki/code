/* main.cpp -- main module
 *
 *			Ryan McDougall -- 2010
 */

#include <iostream>
#include <tr1/functional>
#include <tr1/memory>
#include <exception>
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <cassert>

#include "type_list.h"

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

//=============================================================================
// contains callable objects and associated filters
template <typename Message>
struct call_state
{
    typedef Message message_type;

    typedef function <void(message_type)> slot_type;
    typedef function <bool(message_type)> filter_type;

    typedef vector <filter_type> filter_list;

    call_state (slot_type s) 
        : slot (s)
    {}

    virtual ~call_state ()
    {}

    slot_type   slot;
    filter_list filters;
};

//=============================================================================
// contains the status of the connection
// slot_type will be copied, so functors should define copy constructor

template <typename Message>
struct connection_state : public call_state <Message>
{
    typedef Message message_type;

    typedef call_state <message_type> call_state_type;
    typedef typename call_state_type::slot_type slot_type;
    
    connection_state (slot_type s)
        : call_state <Message> (s), disconnect (false), block (false)
    {}

    virtual ~connection_state ()
    {}

    bool disconnect;
    bool block;
};

//=============================================================================
// simple interface for the connection state
// owns connection_state through shared pointer

template <typename Message>
class connection
{
    public:
        typedef Message message_type;

        typedef call_state <message_type> call_state_type;
        typedef typename call_state_type::slot_type slot_type;
        typedef typename call_state_type::filter_type filter_type;

        typedef connection_state <message_type> connection_state_type;
        typedef shared_ptr <connection_state_type> connection_state_pointer;

        connection (const string &n, connection_state_type *s)
            : name_ (n), state_ (s)
        {}

        connection (const string &n, connection_state_pointer s)
            : name_ (n), state_ (s)
        {}

        void disconnect () { state_-> disconnect = true; }
        void block () { state_-> block = true; }
        void unblock () { state_-> block = false; }

        void filter (filter_type f) { state_-> filters.push_back (f); }

    private:
        string                      name_;
        connection_state_pointer    state_;
};

//=============================================================================
// contains the callable connection state associated with a signal
// slot_type will be copied, so functors should define copy constructor

template <typename Message>
struct emitter_state
{
    typedef Message message_type;

    typedef connection_state <message_type> connection_state_type;
    typedef shared_ptr <connection_state_type> connection_state_pointer;
    typedef weak_ptr <connection_state_type> connection_state_weak_pointer;
    typedef vector <connection_state_weak_pointer> connection_state_list;

    typedef call_state <message_type> call_state_type;
    typedef vector <call_state_type> call_state_list;

    emitter_state () {}
    ~emitter_state () {}

    call_state_list get_call_state ()
    {
        call_state_list active;

        connection_state_weak_pointer conn_weak;

        typename connection_state_list::iterator i = connections.begin();
        typename connection_state_list::iterator e = connections.end();

        for (; i != e; ++i) 
        {
            conn_weak = *i;

            if (!conn_weak.expired())
            {
                connection_state_pointer conn (conn_weak.lock ());

                if (!conn-> disconnect && !conn-> block) 
                    active.push_back (*conn);
            }
        }

        return active;
    }

    connection_state_list connections;
};

//=============================================================================
// calls slots associated with a signal on its behalf
// owns emitter_state through shared pointer
// slot_type will be copied, so functors should define copy constructor
// TODO: create a queued emitter that queues call_state to be executed later

template <typename Message>
class emitter
{
    public:
        typedef Message message_type;

        typedef call_state <message_type> call_state_type;
        typedef emitter_state <message_type> emitter_state_type;
        typedef shared_ptr <emitter_state_type> emitter_state_pointer;

        typedef typename call_state_type::slot_type slot_type;
        typedef typename call_state_type::filter_type filter_type;
        typedef typename call_state_type::filter_list filter_list;

        typedef typename emitter_state_type::call_state_list call_state_list;
        
        emitter (emitter_state_type *s)
            : state_ (s)
        {}

        emitter (emitter_state_pointer s)
            : state_ (s)
        {}

        virtual void operator () (Message data)
        {
            call_state_list active = state_-> get_call_state ();

            typename call_state_list::iterator conn = active.begin();
            typename call_state_list::iterator end = active.end();

            for (; conn != end; ++conn) 
            {
                typename filter_list::iterator f = conn-> filters.begin();
                typename filter_list::iterator e = conn-> filters.end();
                bool filtered = false;

                for (; f != e; ++f)
                    if (filtered = (*f) (data))
                        break;

                if (filtered)
                    break;

                conn-> slot (data);
            }
        }

    private:
        emitter_state_pointer   state_;
};

//=============================================================================
// simple interface for emitter

template <typename Message>
class signal
{
    public:
        typedef Message message_type;
        typedef emitter <message_type> emitter_type;
        typedef emitter_state <message_type> emitter_state_type;
        typedef shared_ptr <emitter_state_type> emitter_state_pointer;

        signal (const string &n, emitter_state_type *s)
            : name_ (n), emitter_ (s)
        {}

        signal (const string &n, emitter_state_pointer s)
            : name_ (n), emitter_ (s)
        {}

        void emit (message_type data) { emitter_ (data); }

    private:
        string          name_;
        emitter_type    emitter_;
};

//=============================================================================

class signal_error : public exception
{
    public:
        signal_error (const string &msg) : msg_ (msg) {}
        ~signal_error () throw () {}
        const char *what () const throw () { return msg_.c_str(); }

    private:
        const string msg_;
};

//=============================================================================
// factory for signals and connecitons
// enforces uniqueness of signal names
// weak-references emitter_state to add new connections
// TODO: not thread safe

class switchboard_base {};

template <typename Message>
class switchboard : public switchboard_base
{
    typedef Message message_type;

    typedef signal <message_type> signal_type;
    typedef connection <message_type> connection_type;
    typedef emitter_state <message_type> emitter_state_type;

    typedef typename emitter_state_type::connection_state_type connection_state_type;
    typedef typename emitter_state_type::connection_state_pointer connection_state_pointer;
    typedef typename emitter_state_type::connection_state_weak_pointer connection_state_weak_pointer;

    typedef shared_ptr <emitter_state_type> emitter_state_pointer;
    typedef weak_ptr <emitter_state_type> emitter_state_weak_pointer;

    typedef map <string, emitter_state_weak_pointer> signal_state_type;

    public:
        switchboard ()
        {}

        signal_type get_signal (string name)
        {
            emitter_state_pointer emit (signal_add_if_missing_ (name));

            return signal_type (name, emit);
        }

        template <typename Callable>
        connection_type connect (string name, Callable func)
        {
            emitter_state_pointer emit (signal_exception_if_missing_ (name));

            connection_state_pointer conn (new connection_state_type (func));
            emit-> connections.push_back (conn);

            return connection_type (name, conn);
        }

    private:
        emitter_state_pointer signal_add_if_missing_ (const string &name)
        {
            emitter_state_pointer state;

            if (!signal_state_.count (name))
                signal_state_.insert (make_pair 
                        (name, emitter_state_weak_pointer ()));
            
            typename signal_state_type::iterator i = signal_state_.find (name);

            if (i-> second.expired())
            {
                state = emitter_state_pointer (new emitter_state_type);
                i-> second = state;
            }
            else
                state = emitter_state_pointer (i-> second);

            return state;
        }

        emitter_state_pointer signal_exception_if_missing_ (const string &name)
        {
            emitter_state_pointer state;

            typename signal_state_type::iterator i = signal_state_.find (name);

            if ((i == signal_state_.end()) || i-> second.expired())
                throw signal_error ("signal missing: " + name);
            else
                state = emitter_state_pointer (i-> second);

            return state;
        }

    private:
        signal_state_type       signal_state_;
};

//=============================================================================
// type-safe interface to switchboard
// TODO: thread safe?

class signal_manager_base {};

template <typename TypeList>
class signal_manager : public signal_manager_base
{
    public:
        typedef TypeList message_type_list;
        static const size_t N_MESSAGE_TYPES = type_list::length <message_type_list>::result;

        signal_manager ()
        {
            initialize_switchboards_();
        }

        template <typename CopyTypeList>
            signal_manager (const signal_manager <CopyTypeList> &r)
            {
                initialize_switchboards_();

                assert ((type_list::extends <TypeList, CopyTypeList>::result));

                const size_t N = type_list::length <CopyTypeList>::result;
                for (size_t i=0; i < N; ++i)
                {
                    dispatchers_[i].reset ();
                    dispatchers_[i] = r.dispatchers_[i];
                }
            }

        template <typename Message>
            signal <Message> get_signal (string name)
            {
                switchboard <Message> *sb = get_switchboard_ <Message> ();
                return sb-> get_signal (name);
            }

        template <typename Message, typename Callable>
            connection <Message> connect (string name, Callable func)
            {
                switchboard <Message> *sb = get_switchboard_ <Message> ();
                return sb-> connect (name, func);
            }

    private:
        template <typename Message>
            switchboard <Message> *get_switchboard_ ()
            {
                 const int index = type_list::index_of <message_type_list, Message>::result;

                 if (index < 0) throw signal_error 
                     (string ("no switchboard for message ") + typeid(Message).name());

                 return static_cast <switchboard <Message> *> (dispatchers_ [index].get());
            }

        void initialize_switchboards_ ()
        {
            // allocate switchboard<T>s
            switchboard_base *sb [N_MESSAGE_TYPES];
            type_list::allocate <message_type_list, switchboard, switchboard_base> allocation (sb);

            // take ownership
            for (size_t i=0; i < N_MESSAGE_TYPES; ++i)
                dispatchers_[i].reset (sb[i]);
        }

    public:
        shared_ptr <switchboard_base> dispatchers_ [N_MESSAGE_TYPES];
};

//=============================================================================

// message type
struct A
{
    A (int v) : value (v) {}
    int value;
};

// functor handler
struct handler_type
{
    handler_type (int v) : b (v) {}

    void operator() (A a)
    {
        cout << "handler got: " << a.value << " and " << b << endl;
    }

    int b;
};

// functor filter
struct filter_type
{
    filter_type (int v) : b (v) {}

    bool operator() (A a)
    {
        bool filtered = (a.value == b);

        cout << "filter got: " << a.value << " and " << b << endl;
        cout << "filtered: " << filtered << endl;

        return filtered;
    }

    int b;
};

// callback handler
void callback (A a, int b)
{
    cout << "callback got: " << a.value << " " << b << endl;
}

// callback handler
void err (int b)
{
    cout << "error got: " << b << endl;
}

//=============================================================================
// convenience classes to manage extending signal_manager with new messages
// should be copied or #defined

struct signal_manager_factory_1
{
    // message type list
    typedef TYPE_LIST_1 (A) message_type_list;

    // signal manager for message type list
    typedef signal_manager <message_type_list> type;

    // default factory method
    static signal_manager <message_type_list> create ()
    {
        return signal_manager <message_type_list> ();
    }
    
    // extension factory method
    template <typename TypeList>
    static signal_manager <message_type_list> extend (const signal_manager <TypeList> &r)
    {
        return signal_manager <message_type_list> (r);
    }
};

struct signal_manager_factory_2
{
    // message type list
    typedef TYPE_LIST_2 (A, int) message_type_list;

    // signal manager for message type list
    typedef signal_manager <message_type_list> type;

    // default factory method
    static signal_manager <message_type_list> create ()
    {
        return signal_manager <message_type_list> ();
    }
    
    // extension factory method
    template <typename TypeList>
    static signal_manager <message_type_list> extend (const signal_manager <TypeList> &r)
    {
        return signal_manager <message_type_list> (r);
    }
};

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    // test manager extension
    signal_manager_factory_1::type sm1 = signal_manager_factory_1::create ();
    signal_manager_factory_2::type sm2 = signal_manager_factory_2::extend (sm1);
    
    // test signal creation
    signal <A> sig1 = sm1.get_signal <A> ("my signal 1");
    signal <A> sig11 = sm1.get_signal <A> ("my signal 1");
    signal <A> sig2 = sm1.get_signal <A> ("my signal 2");

    {
        // test life-time management (shared_ptr)
        signal <int> s = sm2.get_signal <int> ("my signal 2");
        connection <int> c = sm2.connect <int> ("my signal 2", err);
        s.emit (101);
    }

    A a (5); handler_type handler (1);

    // test type safety
    connection <A> c1 = sm1.connect <A> ("my signal 1", handler);
    connection <A> c2 = sm1.connect <A> ("my signal 2", bind (callback, _1, 6));

    // test copy by value semantics
    handler.b = 3;

    // test blocking
    c1.block();
    sig1.emit (a);
    c1.unblock();

    // test duplicate emission
    sig11.emit (a);

    // test filtering
    c2.filter (filter_type (9));
    a.value = 9;
    sig2.emit (a);

    cout << "leaving." << endl;
    return 0;
}
