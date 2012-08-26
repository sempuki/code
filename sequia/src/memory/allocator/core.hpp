#ifndef _ALLOCATOR_CORE_HPP_
#define _ALLOCATOR_CORE_HPP_


namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            template <typename Type, typename ...Args>
            using get_state_type = typename Type::template state_type <Args...>;

            template <typename Type, typename ...Args>
            using get_concrete_type = typename Type::template concrete_type <Args...>;

            template <typename Type, typename ...Args>
            using get_concrete_base_type = get_concrete_type <Type, get_state_type <Type, Args...>, Args...>;

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
