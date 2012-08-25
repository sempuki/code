#ifndef _TERMINAL_ALLOCATOR_HPP_
#define _TERMINAL_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements state storage and terminates allocator recursion
            
            template <typename State>
            class terminal
            {
                public:
                    using state_type = State;

                public:
                    terminal () = default;

                    terminal (state_type const &state) :
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
