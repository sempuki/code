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
        
        template <int StateID, typename State>
        struct state_descriptor
        {
            constexpr static int    state_id = StateID;
            typedef State           state_type;
        };

        //---------------------------------------------------------------------

        template <typename Context, int StateID, typename ...States>
        struct state_enumerator 
        {
            template <typename Event> 
            inline void activate (Context &, Event const &, core::dispatch_tag <traits::state::null>) {}
            inline void activate (Context &, core::dispatch_tag <traits::state::null>) {}
            inline void deactivate (Context &, core::dispatch_tag <traits::state::null>) {}

            template <typename InitState> 
            inline void initialize (Context &) {}
            inline void terminate (Context &) {}

            template <typename Event> 
            inline void react (Context &, Event const &) {}
        };

        template <typename Context, int StateID, typename State, typename ...States>
        struct state_enumerator <Context, StateID, State, States...> : 
        public state_enumerator <Context, StateID+1, States...>
        {
            typedef state_enumerator <Context, StateID+1, States...> base_type;

            // introduce base type overloads into this namespace
            using base_type::activate;
            using base_type::deactivate;    
            using base_type::initialize;
            using base_type::terminate;
            using base_type::react;

            template <typename Event> 
            inline void activate (Context &ctx, Event const &event, core::dispatch_tag<State>)
            {
                // dispatch trick uses overload lookup to find needed state descriptor
                ctx.template activate <state_descriptor <StateID, State>> (event);
            }

            inline void activate (Context &ctx, core::dispatch_tag<State>)
            {
                // dispatch trick uses overload lookup to find needed state descriptor
                ctx.template activate <state_descriptor <StateID, State>> ();
            }

            inline void deactivate (Context &ctx, core::dispatch_tag<State>)
            {
                // dispatch trick uses overload lookup to find needed state descriptor
                ctx.template deactivate <state_descriptor <StateID, State>> ();
            }

            template <typename InitState> 
            inline void initialize (Context &ctx)
            {
                using std::is_same;
                using core::dispatch_tag;

                if (is_same <InitState, State>::value)
                    activate (ctx, dispatch_tag <State>());

                base_type::template initialize <State> (ctx);
            }

            inline void terminate (Context &ctx)
            {
                using core::dispatch_tag;

                if (ctx.template is_active <state_descriptor <StateID, State>> ())
                    deactivate (ctx, dispatch_tag <State>());

                base_type::terminate (ctx);
            }

            template <typename Event> 
            inline void react (Context &ctx, Event const &event)
            {
                using std::is_same;
                using core::dispatch_tag;
                using traits::state::transition;

                typedef State                                   CurrState;
                typedef typename transition<State, Event>::next NextState;
                typedef traits::state::null                     NullState;

                if (!is_same <NextState, NullState>::value && 
                    ctx.template is_active <state_descriptor <StateID, State>> ())
                {
                    deactivate (ctx, dispatch_tag <CurrState>());
                    activate (ctx, event, dispatch_tag <NextState>());
                }

                base_type::template react <Event> (ctx, event);
            }
        };

        //---------------------------------------------------------------------

        template <typename ...States>
        class singular_context
        {
            public:
                constexpr static size_t nstates = sizeof...(States);

                template <typename StateDescriptor>
                inline bool is_active () const
                {
                    return curr_active == StateDescriptor::state_id;
                }

                template <typename StateDescriptor>
                inline void activate ()
                {
                    typedef typename StateDescriptor::state_type State;
                    new (reinterpret_cast <void *> (buffer)) State ();
                    
                    next_active = StateDescriptor::state_id;
                }

                template <typename StateDescriptor, typename Event>
                inline void activate (Event const &event)
                {
                    typedef typename StateDescriptor::state_type State;
                    new (reinterpret_cast <void *> (buffer)) State (event);
                    
                    next_active = StateDescriptor::state_id;
                }

                template <typename StateDescriptor>
                inline void deactivate ()
                {
                    typedef typename StateDescriptor::state_type State;
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
                typedef singular_context <Default, States...> context_type;

                singular_machine () 
                { 
                    states_.template initialize <Default> (context_); 
                }

                ~singular_machine () 
                { 
                    states_.terminate (context_); 
                }

                template <typename Event> 
                void react (Event const &event) 
                { 
                    context_.flip ();
                    states_.template react <Event> (context_, event); 
                }

            private:
                context_type                                            context_;
                state_enumerator <context_type, 0, Default, States...>  states_;
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
