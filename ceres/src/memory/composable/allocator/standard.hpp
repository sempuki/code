#ifndef _STANDARD_ALLOCATOR_HPP_
#define _STANDARD_ALLOCATOR_HPP_

namespace ceres
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements standard allocator semantics
            // Fulfills stateful allocator concept
            // Fulfills composable allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <typename ConcreteValue = std::false_type, 
                     typename ConcreteState = std::false_type>

            class standard : 
                public std::allocator<ConcreteValue>,
                public terminal<std::false_type>
            {
                public:
                    using base_type = terminal<std::false_type>;
                    using state_type = std::false_type;
                    using value_type = ConcreteValue;

                public:
                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U>
                    struct rebind { using other = standard<U>; };

                    template <typename U, typename S>
                    struct reify { using other = standard<U, S>; };

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
