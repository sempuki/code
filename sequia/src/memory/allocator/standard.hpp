#ifndef _STANDARD_ALLOCATOR_HPP_
#define _STANDARD_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements fixed-buffer semantics
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <typename T>
            class standard : public std::allocator<T>
            {
                public:
                    using base_type = std::false_type;
                    
                    using value_type = T;
                    using state_type = base_state<T>;

                public:
                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U>
                    struct rebind 
                    { 
                        using other = standard<U>; 
                    };

                public:
                    // default constructor
                    standard () = default;

                    // copy constructor
                    standard (standard const &copy) = default;

                    // stateful constructor
                    explicit standard (state_type const &state) {}

                    // destructor
                    ~standard () = default;
            };
        }
    }
}

#endif
