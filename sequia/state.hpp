#ifndef _STATE_HPP_
#define _STATE_HPP_

namespace traits
{
    namespace state
    {
        struct null_transition {};

        template <typename State, typename Event>
        struct transition
        {
            constexpr static bool exists = false;
            typedef null_transition next;
        };
    }
}

namespace sequia
{
    namespace state
    {
        //=========================================================================
        
        class activator
        {
            public:
                activator (uint8_t *mem)
                    : mem_ (mem) {}

                template <typename State>
                inline void activate ()
                { 
                    new (mem_) State (); 
                }
                
                template <typename State, typename Event>
                inline void activate (Event const &event) 
                { 
                    new (mem_) State (event); 
                }
                
                template <typename State>
                inline void deactivate () 
                { 
                    mem_->~State(); 
                }

            private:
                uint8_t *mem_;
        };

        //-------------------------------------------------------------------------
        
        template <size_t N>
        class singular_context : private activator
        {
            public:
                singular_context (uint8_t *mem)
                    : activator (mem) {}

                bool is_active (size_t stateid) const
                { 
                    return stateid == active_; 
                }
                
                template <typename State>
                void activate (size_t stateid)
                { 
                    activator::activate <State> ();
                    active_ = stateid;
                }
                
                template <typename State, typename Event>
                void activate (size_t stateid, Event const &event) 
                { 
                    activator::activate <State, Event> (event);
                    active_ = stateid;
                }
                
                template <typename State>
                void deactivate (size_t stateid) 
                { 
                    activator::deactivate <State> ();
                    active_ = N;
                }

            private:
                size_t active_ = N;
        };

        //-------------------------------------------------------------------------

        template <size_t N>
        class parallel_context : private activator
        {
            public:
                parallel_context (uint8_t *mem)
                    : activator (mem) {}

                bool is_active (size_t stateid) const
                { 
                    return active_ [stateid]; 
                }

                template <typename State>
                void activate (size_t stateid)
                { 
                    activator::activate <State> ();
                    active_ = stateid;
                }
                
                template <typename State, typename Event>
                void activate (size_t stateid, Event const &event) 
                { 
                    activator::activate <State, Event> (event);
                    active_.set (stateid);
                }
                
                template <typename State>
                void deactivate (size_t stateid) 
                { 
                    activator::deactivate <State> ();
                    active_.reset (stateid);
                }

            private:
                std::bitset<N> active_;
        };

        //=========================================================================

        template <int StateID, typename Context, typename ...States>
        struct machine_base 
        {
            template <typename Event>
            inline void react (Context &ctx, Event const &event) {}
        };

        template <int StateID, typename Context, typename State, typename ...States>
        struct machine_base <StateID, Context, State, States...> : 
        public machine_base <StateID+1, Context, States...>
        {
            typedef machine_base <StateID+1, Context, States...> parent_type;

            template <typename Event>
            inline void react (Context &ctx, Event const &event)
            {
                using traits::state::transition;

                if (transition<State, Event>::exists && ctx.is_active (StateID))
                {
                    typedef typename transition<State, Event>::next Next;

                    ctx.deactivate <State> (StateID);
                    ctx.activate <Next, Event> (StateID, event);
                }
                else
                {
                    parent_type::react (ctx, event);
                }
            }
        };

        //-------------------------------------------------------------------------
        
        template <typename Context, typename Initial, typename ...States>
        class machine : public machine_base <0, Context, Initial, States...>
        {
            typedef machine_base <0, Context, Initial, States...> parent_type;

            public:
                machine () : ctx_ (mem_) { ctx_.activate <Initial> (0); }

                template <typename Event>
                void react (Event const &event) { parent_type::react (ctx_, event); }

            private:
                Context ctx_;
                uint8_t mem_ [core::max_type_size<States...>()];
        };
    }
}

#endif
