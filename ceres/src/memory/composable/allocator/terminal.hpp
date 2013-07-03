#ifndef _TERMINAL_ALLOCATOR_HPP_
#define _TERMINAL_ALLOCATOR_HPP_

namespace ceres
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements state storage and terminates allocator recursion
            
            namespace impl
            {
                template <typename State>
                class terminal : public stateful<State> {};
            }

            struct terminal
            {
                template <typename S, typename T>
                using concrete_type = impl::terminal<S>;
            };
        }
    }
}

#endif
