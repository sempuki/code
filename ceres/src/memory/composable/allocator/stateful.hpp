#ifndef _STATEFUL_ALLOCATOR_HPP_
#define _STATEFUL_ALLOCATOR_HPP_

namespace ceres
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements state storage and access
            
            template <typename State>
            class stateful
            {
                public:
                    using state_type = State;

                public:
                    stateful () = default;

                    stateful (state_type const &state) :
                        instance_state {state} {}

                public:
                    state_type &access_state () { return instance_state; }
                    state_type const &access_state () const { return instance_state; }

                private:
                    state_type  instance_state;
            };
        }
    }
}

#endif
