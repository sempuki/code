#ifndef _ALLOCATOR_CORE_HPP_
#define _ALLOCATOR_CORE_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            template <typename T>
            struct base_state 
            {
                typedef T   value_type;

                base_state (size_t size) :
                    arena {size} {}

                base_state (buffer<T> const &buf) :
                    arena {buf} {}

                template <size_t N>
                base_state (static_buffer<T, N> const &buf) :
                    arena {buf} {}

                template <typename U>
                base_state (base_state<U> const &copy) :
                    arena {nullptr, copy.arena.size} {}

                buffer<T> arena;
            };
        }
    }
}

#endif
