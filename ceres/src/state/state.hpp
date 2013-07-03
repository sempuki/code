#ifndef _STATE_HPP_
#define _STATE_HPP_
        
#include <typeindex>

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


        // To implement state transitions specialize transition template 
        // within the implemented translation unit as follows:
        //
        // namespace traits
        // {
        //     namespace state
        //     {
        //         template <> struct transition <State1, Event1> { typedef State2 next; };
        //         template <> struct transition <State2, Event2> { typedef State3 next; };
        //         template <> struct transition <State3, Event3> { typedef State1 next; };
        //     }
        // }

    }
}

namespace ceres
{
    namespace state
    {
        //=====================================================================
        
        //---------------------------------------------------------------------
        // Mapping of state types to constant integer IDs for use in CRTP types

        template <int StateID, typename State>
        struct state_descriptor
        {
            constexpr static int    state_id = StateID;
            typedef State           state_type;
        };

        //---------------------------------------------------------------------
        // Creates a list of descriptors from variadic template list

        template <int StateID, typename ...States>
        struct state_enumerator 
        {
            typedef traits::state::null descriptor_type;
            typedef traits::state::null next_type;
        };

        template <int StateID, typename State, typename ...States>
        struct state_enumerator <StateID, State, States...> : 
        public state_enumerator <StateID+1, States...>
        {
            typedef state_descriptor <StateID, State>       descriptor_type;
            typedef state_enumerator <StateID+1, States...> next_type;
        };

        //---------------------------------------------------------------------
        // Recursively creates a single class with per-state overloaded state 
        // de/activation methods; dependent objects must pre-declare before using
            
        template <typename Context, 
                 typename CurrEnum,
                 typename NextEnum = typename CurrEnum::next_type>

        struct state_machine_activator : 
        public state_machine_activator <Context, NextEnum>
        {
            typedef state_machine_activator <Context, NextEnum> base;

            typedef typename CurrEnum::descriptor_type          Descriptor;
            typedef typename Descriptor::state_type             State;

            // introduce base type overloads into this namespace
            using base::activate;
            using base::deactivate;    

            template <typename Event> 
            inline void activate (Context &ctx, Event const &event, core::dispatch_tag<State>)
            {
                // dispatch trick uses overload lookup to find needed state descriptor
                ctx.template activate <Descriptor> (event);
            }

            inline void activate (Context &ctx, core::dispatch_tag<State>)
            {
                // dispatch trick uses overload lookup to find needed state descriptor
                ctx.template activate <Descriptor> ();
            }

            inline void deactivate (Context &ctx, core::dispatch_tag<State>)
            {
                // dispatch trick uses overload lookup to find needed state descriptor
                ctx.template deactivate <Descriptor> ();
            }
        };
        
        template <typename Context, typename CurrEnum>
        struct state_machine_activator <Context, CurrEnum, traits::state::null>
        {
            template <typename Event> 
            inline void activate (Context &, Event const &, core::dispatch_tag <traits::state::null>) {}
            inline void activate (Context &, core::dispatch_tag <traits::state::null>) {}
            inline void deactivate (Context &, core::dispatch_tag <traits::state::null>) {}
        };

        //---------------------------------------------------------------------
        // Uses activator class to implement state init/term and event-reactions 
            
        template <typename Context, 
                 typename BaseEnum, 
                 typename CurrEnum = BaseEnum, 
                 typename NextEnum = typename CurrEnum::next_type>

        struct state_machine_reactor : 
        public state_machine_reactor <Context, BaseEnum, NextEnum>
        {
            typedef state_machine_reactor <Context, BaseEnum, NextEnum> base;
            typedef state_machine_activator <Context, BaseEnum> activator;

            // introduce base type overloads into this namespace
            using base::activate;
            using base::deactivate;    

            typedef typename CurrEnum::descriptor_type  Descriptor;
            typedef typename Descriptor::state_type     State;

            template <typename InitialState> 
            inline void initialize (Context &ctx)
            {
                using std::is_same;
                using core::dispatch_tag;

                if (is_same <InitialState, State>::value)
                    activate (ctx, dispatch_tag <State> ());

                base::template initialize <State> (ctx);
            }

            inline void terminate (Context &ctx)
            {
                using core::dispatch_tag;

                if (ctx.template is_active <Descriptor> ())
                    deactivate (ctx, dispatch_tag <State> ());

                base::terminate (ctx);
            }

            template <typename Event> 
            inline void react (Context &ctx, Event const &event)
            {
                using std::is_same;
                using core::dispatch_tag;
                using traits::state::null;
                using traits::state::transition;

                typedef null                                        NullState;
                typedef State                                       CurrState;
                typedef typename transition <State, Event>::next    NextState;

                if (!is_same <NextState, NullState>::value && 
                    ctx.template is_active <Descriptor> ())
                {
                    deactivate (ctx, dispatch_tag <CurrState> ());
                    activate (ctx, event, dispatch_tag <NextState> ());
                }

                base::template react <Event> (ctx, event);
            }
        };

        template <typename Context, typename BaseEnum, typename CurrEnum>
        struct state_machine_reactor <Context, BaseEnum, CurrEnum, traits::state::null> :
        public state_machine_activator <Context, BaseEnum>
        {
            typedef state_machine_activator <Context, BaseEnum> base;

            // introduce base type overloads into this namespace
            using base::activate;
            using base::deactivate;    

            template <typename InitState> 
            inline void initialize (Context &) {}
            inline void terminate (Context &) {}

            template <typename Event> 
            inline void react (Context &, Event const &) {}
        };
            
        //---------------------------------------------------------------------
        // holds all dynamic state context: active state id and state object itself

        template <typename ...States>
        class singular_context
        {
            public:
                constexpr static size_t nstates = sizeof...(States);

                template <typename Descriptor>
                inline bool is_active () const
                {
                    return curr_active == Descriptor::state_id;
                }

                template <typename Descriptor>
                inline void activate ()
                {
                    typedef typename Descriptor::state_type State;
                    new (reinterpret_cast <void *> (buffer)) State ();
                    
                    next_active = Descriptor::state_id;
                }

                template <typename Descriptor, typename Event>
                inline void activate (Event const &event)
                {
                    typedef typename Descriptor::state_type State;
                    new (reinterpret_cast <void *> (buffer)) State (event);
                    
                    next_active = Descriptor::state_id;
                }

                template <typename Descriptor>
                inline void deactivate ()
                {
                    typedef typename Descriptor::state_type State;
                    reinterpret_cast <State *> (buffer)-> ~State();
                }

                void commit ()
                {
                    curr_active = next_active;
                }

            private:
                size_t  curr_active = nstates;
                size_t  next_active = nstates;
                uint8_t buffer [core::max_type_size<States...>()];
        };

        //---------------------------------------------------------------------

        template <typename Default, typename ...States>
        class singular_machine
        {
            public:
                typedef singular_context <Default, States...>           Context;
                typedef state_enumerator <0, Default, States...>        Enumeration;
                typedef state_machine_reactor <Context, Enumeration>    Reactor;

                singular_machine () 
                { 
                    reactor_.template initialize <Default> (context_); 
                    context_.commit ();
                }

                ~singular_machine () 
                { 
                    reactor_.terminate (context_); 
                }

                template <typename Event> 
                void react (Event const &event) 
                { 
                    reactor_.template react <Event> (context_, event); 
                    context_.commit ();
                }

            private:
                Context     context_;
                Reactor     reactor_;
        };

        //---------------------------------------------------------------------
        
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
