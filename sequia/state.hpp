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
    }
}

namespace sequia
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
            typedef traits::state::null base_type;
            typedef traits::state::null descriptor_type;
        };

        template <int StateID, typename State, typename ...States>
        struct state_enumerator <StateID, State, States...> : 
        public state_enumerator <StateID+1, States...>
        {
            typedef state_enumerator <StateID+1, States...> base_type;
            typedef state_descriptor <StateID, State>       descriptor_type;
        };

        //---------------------------------------------------------------------
        // Recursively creates a single class with per-state overloaded state 
        // de/activation methods; dependent objects must pre-declare before using
            
        template <typename Context, typename Enumerator, typename Next = typename Enumerator::base_type>
        struct state_machine_activator : 
        public state_machine_activator <Context, Next>
        {
            typedef state_machine_activator <Context, Next, typename Next::base_type> base;

            typedef typename Enumerator::descriptor_type   Descriptor;
            typedef typename Descriptor::state_type        State;

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
        
        template <typename Context, typename Enumerator>
        struct state_machine_activator <Context, Enumerator, traits::state::null>
        {
            template <typename Event> 
            inline void activate (Context &, Event const &, core::dispatch_tag <traits::state::null>) {}
            inline void activate (Context &, core::dispatch_tag <traits::state::null>) {}
            inline void deactivate (Context &, core::dispatch_tag <traits::state::null>) {}
        };

        //---------------------------------------------------------------------
        // Uses activator class to implement init/termination and state reactions 
            
        template <typename Context, typename Enumerator, typename Next = typename Enumerator::base_type>
        struct state_machine_reactor : 
        public state_machine_reactor <Context, Next>,
        public state_machine_activator <Context, Enumerator>
        {
            typedef state_machine_reactor <Context, Next>           base;
            typedef state_machine_activator <Context, Enumerator>   activator;

            typedef typename Enumerator::descriptor_type    Descriptor;
            typedef typename Descriptor::state_type         State;

            // introduce parent/base type overloads into this namespace
            using base::initialize;
            using base::terminate;
            using base::react;

            template <typename InitialState> 
            inline void initialize (Context &ctx)
            {
                using std::is_same;
                using core::dispatch_tag;

                if (is_same <InitialState, State>::value)
                    activator::activate (ctx, dispatch_tag <State> ());

                base::template initialize <State> (ctx);
            }

            inline void terminate (Context &ctx)
            {
                using core::dispatch_tag;

                if (ctx.template is_active <Descriptor> ())
                    activator::deactivate (ctx, dispatch_tag <State> ());

                base::terminate (ctx);
            }

            template <typename Event> 
            inline void react (Context &ctx, Event const &event)
            {
                using std::is_same;
                using core::dispatch_tag;
                using traits::state::transition;

                typedef State                                       CurrState;
                typedef typename transition <State, Event>::next    NextState;
                typedef traits::state::null                         NullState;

                if (!is_same <NextState, NullState>::value && 
                    ctx.template is_active <Descriptor> ())
                {
                    activator::deactivate (ctx, dispatch_tag <CurrState> ());
                    activator::activate (ctx, event, dispatch_tag <NextState> ());
                }

                base::template react <Event> (ctx, event);
            }
        };

        template <typename Context, typename Enumerator>
        struct state_machine_reactor <Context, Enumerator, traits::state::null>
        {
            template <typename InitState> 
            inline void initialize (Context &) {}
            inline void terminate (Context &) {}

            template <typename Event> 
            inline void react (Context &, Event const &) {}
        };
            
        //---------------------------------------------------------------------

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

                void flip ()
                {
                    using std::swap;

                    swap (curr_active, next_active);
                    next_active = curr_active;
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
                typedef state_enumerator <0, Default, States...>        Enumerator;
                typedef state_machine_reactor <Context, Enumerator>     Reactor;

                singular_machine () 
                { 
                    reactor_.template initialize <Default> (context_); 
                }

                ~singular_machine () 
                { 
                    reactor_.terminate (context_); 
                }

                template <typename Event> 
                void react (Event const &event) 
                { 
                    context_.flip ();
                    reactor_.template react <Event> (context_, event); 
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
