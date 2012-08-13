#ifndef _ALLOCATOR_CORE_HPP_
#define _ALLOCATOR_CORE_HPP_


namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            namespace detail
            {
                template <typename Delegator, typename T, typename S>
                using base = typename Delegator::template 
                    reify<T, S>::other;

                template <typename Delegator, typename T>
                using concrete_base = typename Delegator::template 
                    reify<T, typename core::rebinder<Delegator, T>::state_type>::other;
            }

            template <typename T>
            struct basic_state 
            {
                using value_type = T;

                basic_state () = default;

                basic_state (size_t size) :
                    arena {size} {}

                basic_state (buffer<T> &buf) :
                    arena {buf} {}

                template <size_t N>
                basic_state (static_buffer<T, N> &buf) :
                    arena {buf} {}

                template <typename U>
                basic_state (basic_state<U> const &copy) :
                    arena {nullptr, copy.arena.size} {}

                ~basic_state() = default;

                buffer<T> arena;
            };
        }
    }
}

#endif
