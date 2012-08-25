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
            
            struct terminal
            {
                template <typename S, typename T>
                using concrete_type = impl::terminal<S>;
            };

            namespace impl
            {
                template <typename State>
                class terminal :
                    stateful<State> {};
            }
        }
    }
}

#endif
