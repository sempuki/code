#ifndef _STATE_HPP_
#define _STATE_HPP_
        
namespace traits
{
    namespace state
    {
        struct null {};

        template <typename State, typename Event>
        struct transition
        {
            typedef null next;
        };
    }
}

namespace sequia
{
    namespace state
    {
        //=========================================================================

        template <typename RootMachine, typename TargetState>
        struct activator
        {
            RootMachine &machine;

            activator (RootMachine &m) 
                : machine (m) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;

                typedef typename traits::state::null        Null;
                typedef typename CurrMachine::state_type    CurrState;
                typedef typename CurrMachine::base_type     BaseMachine;
                
                if (is_same <CurrState, TargetState>::value)
                    machine.set_active (CurrMachine::stateid);

                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //---------------------------------------------------------------------

        template <typename RootMachine, typename TargetState>
        struct deactivator
        {
            RootMachine &machine;

            deactivator (RootMachine &m) 
                : machine (m) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;

                typedef typename traits::state::null        Null;
                typedef typename CurrMachine::state_type    CurrState;
                typedef typename CurrMachine::base_type     BaseMachine;

                if (is_same <CurrState, TargetState>::value)
                    machine.set_inactive (CurrMachine::stateid);

                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //---------------------------------------------------------------------

        template <typename RootMachine, typename TargetState>
        struct default_constructor
        {
            RootMachine &machine; 

            default_constructor (RootMachine &m)
                : machine (m) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;

                typedef typename traits::state::null        Null;
                typedef typename CurrMachine::state_type    CurrState;
                typedef typename CurrMachine::base_type     BaseMachine;

                if (is_same <CurrState, TargetState>::value)
                    new (machine.buffer()) CurrState (); 
                
                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //---------------------------------------------------------------------

        template <typename RootMachine, typename TargetState, typename Event>
        struct constructor
        {
            RootMachine &machine; 
            Event const &event;

            constructor (RootMachine &m, Event const &e)
                : machine (m), event (e) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;

                typedef typename traits::state::null        Null;
                typedef typename CurrMachine::state_type    CurrState;
                typedef typename CurrMachine::base_type     BaseMachine;

                if (is_same <CurrState, TargetState>::value)
                    new (machine.buffer()) CurrState (event);
                
                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //---------------------------------------------------------------------

        template <typename RootMachine, typename TargetState>
        struct destructor
        {
            RootMachine &machine; 

            destructor (RootMachine &m)
                : machine (m) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;

                typedef typename traits::state::null        Null;
                typedef typename CurrMachine::state_type    CurrState;
                typedef typename CurrMachine::base_type     BaseMachine;

                if (is_same <CurrState, TargetState>::value)
                    static_cast <CurrState *> (machine.buffer())-> ~CurrState (); 
                
                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //---------------------------------------------------------------------

        template <typename RootMachine, typename TargetState>
        struct active_destructor
        {
            RootMachine &machine; 

            active_destructor (RootMachine &m)
                : machine (m) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;

                typedef typename traits::state::null        Null;
                typedef typename CurrMachine::state_type    CurrState;
                typedef typename CurrMachine::base_type     BaseMachine;

                if (machine.is_active (BaseMachine::stateid))
                    static_cast <CurrState *> (machine.buffer())-> ~CurrState (); 
                
                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //---------------------------------------------------------------------

        template <typename RootMachine, typename Event>
        struct reactor
        {
            RootMachine &machine; 
            Event const &event;

            reactor (RootMachine &m, Event const &e)
                : machine (m), event (e) {}

            template <typename CurrMachine>
            inline void start ()
            {
                using std::is_same;
                using traits::state::transition;

                typedef typename traits::state::null                Null;
                typedef typename CurrMachine::state_type            CurrState;
                typedef typename CurrMachine::base_type             BaseMachine;
                typedef typename transition<CurrState, Event>::next NextState;

                if (!is_same <NextState, Null>::value && 
                    machine.is_active (BaseMachine::stateid))
                {
                    typedef typename RootMachine::base_type RootBase;

                    deactivator <RootMachine, CurrState> {machine}.
                        start <CurrMachine> ();

                    destructor <RootMachine, CurrState> {machine}.
                        start <CurrMachine> ();

                    activator <RootMachine, NextState> {machine}.
                        start <RootBase> ();

                    constructor <RootMachine, NextState, Event> {machine, event}.
                        start <RootBase> ();
                }
                else if (!is_same <BaseMachine, Null>::value)
                    start <BaseMachine> ();
            }
        };
        
        //=========================================================================
        
        template <typename ...States>
        struct singular_context
        {
            constexpr static size_t N = sizeof...(States);

            size_t  active = N;
            uint8_t buffer [core::max_type_size<States...>()];
        };

        template <int StateID, typename ...States>
        struct singular_machine_base
        {
            typedef traits::state::null base_type;
            typedef traits::state::null state_type;
            
            constexpr static int stateid = StateID;
        };

        template <int StateID, typename State, typename ...States>
        struct singular_machine_base <StateID, State, States...> : 
        public singular_machine_base <StateID+1, States...>
        {
            typedef singular_machine_base <StateID+1, States...>    base_type;
            typedef State                                           state_type;
            
            constexpr static int stateid = StateID;
        };

        template <typename Initial, typename ...States>
        class singular_machine : 
            private singular_machine_base <0, Initial, States...>
        {
            typedef singular_machine_base <0, Initial, States...>   base_type;
            typedef singular_machine                                this_type;

            public:
                singular_machine () 
                { 
                    default_constructor <this_type, Initial> {*this}.
                        start <base_type> ();
                }

                ~singular_machine () 
                {
                    active_destructor <this_type> {*this}.
                        start <base_type> ();
                }

                void *buffer () { return ctx_.buffer; }
                void is_active (int stateid) { return ctx_.active == stateid; }
                void set_active (int stateid) { return ctx_.active = stateid; }
                void set_inactive (int stateid) {}

                template <typename Event>
                void react (Event const &event)
                {
                    reactor <this_type, Event> {*this, event}.
                        start <base_type> ();
                }

            private:
                singular_context <Initial, States...> ctx_;
        };

        //-------------------------------------------------------------------------
        
        //template <typename ...States>
        //struct parallel_context
        //{
        //    std::tuple <States...> states;
        //};

        //template <int StateID, typename ...States>
        //struct parallel_machine_base;

        //template <int StateID, typename State, typename ...States>
        //struct parallel_machine_base <StateID, State, States...> : 
        //public parallel_machine_base <StateID+1, States...>
        //{
        //};

        //template <int StateID, typename State, typename ...States>
        //struct parallel_machine <StateID, State, States...> : 
        //public parallel_machine <StateID+1, States...>
        //{
        //};
    }
}

#endif
