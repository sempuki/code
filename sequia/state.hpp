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
            inline void react (Context &ctx, Event const &event) {}
        };

        template <typename Context, int StateID, typename State, typename ...States>
        struct state_enumerator <Context, StateID, State, States...> : 
        public state_enumerator <Context, StateID+1, States...>
        {
            typedef state_enumerator <Context, StateID+1, States...> base_type;

            inline void deactivate (Context &ctx, State *)
            {
                ctx.template deactivate <state_descriptor <StateID, State>> ();
            }

            template <typename Event> 
            inline void activate (Context &ctx, Event const &event, State *)
            {
                ctx.template activate <state_descriptor <StateID, State>> (event);
            }

            template <typename Event> 
            inline void react (Context &ctx, Event const &event)
            {
                using std::is_same;
                using traits::state::transition;

                typedef State                                   CurrState;
                typedef typename transition<State, Event>::next NextState;
                typedef traits::state::null                     NullState;

                if (!is_same <NextState, NullState>::value && 
                    ctx.template is_active <state_descriptor <StateID, State>> ())
                {
                    deactivate (ctx, (CurrState *)0);       // used for overload dispatch only
                    activate (ctx, event, (NextState *)0);  // used for overload dispatch only
                }

                else base_type::react (ctx, event);
            }
        };

        //---------------------------------------------------------------------

        template <typename ...States>
        class singular_context
        {
            public:
                constexpr static size_t nstates = sizeof...(States);

                template <typename StateDescriptor>
                inline void deactivate ()
                {
                    typedef typename StateDescriptor::state_type State;
                    reinterpret_cast <State *> (buffer)-> ~State();
                }

                template <typename StateDescriptor>
                inline void activate ()
                {
                    typedef typename StateDescriptor::state_type State;
                    new (reinterpret_cast <void *> (buffer)) State ();
                    
                    active = StateDescriptor::state_id;
                }

                template <typename StateDescriptor, typename Event>
                inline void activate (Event const &event)
                {
                    typedef typename StateDescriptor::state_type State;
                    new (reinterpret_cast <void *> (buffer)) State (event);
                    
                    active = StateDescriptor::state_id;
                }

                template <typename StateDescriptor>
                inline bool is_active () const
                {
                    return active == StateDescriptor::state_id;
                }

            private:
                size_t  active = nstates;
                uint8_t buffer [core::max_type_size<States...>()];
        };

        //---------------------------------------------------------------------

        template <typename Initial, typename ...States>
        class singular_machine
        {
            public:
                typedef singular_context <Initial, States...> context_type;

                template <typename Event> 
                void react (Event const &event) { states_.react (ctx_, event); }

            private:
                context_type ctx_;
                state_enumerator <context_type, 0, Initial, States...> states_;
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
