#ifndef _CONSTANT_ALLOCATOR_HPP_
#define _CONSTANT_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements scoped allocate-on-construction semantics
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept

            template <typename Delegator, size_t N, typename T>
            class constant : 
                public detail::base<Delegator, T>
            {
                public:
                    using base_type = detail::base<Delegator, T>;
                    using state_type = detail::basestate<Delegator, T>;
                    using value_type = T;
                    
                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U> 
                    struct rebind { using other = constant<Delegator, N, U>; };

                public:
                    // default (stateful) constructor
                    constant () : base_type {state_type {N}} {} 

                    // copy constructor
                    constant (constant const &copy) = default;

                    // stateful constructor
                    explicit constant (state_type const &state) {}

                    // destructor
                    ~constant () = default;
            };
        }
    }
}

#endif
