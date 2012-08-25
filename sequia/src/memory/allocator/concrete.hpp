#ifndef _CONCRETE_ALLOCATOR_HPP_
#define _CONCRETE_ALLOCATOR_HPP_

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
            // Fulfills concrete allocator concept
            
            template <typename Delegator, typename T>

            class concrete : 
                public detail::concrete_base<Delegator, T>
            {
                public:
                    using base_type = detail::concrete_base<Delegator, T>;
                    using value_type = T;

                    struct state_type : 
                        base_type::state_type {};

                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U> 
                    struct rebind { using other = concrete<Delegator, U>; };

                public:
                    // default constructor
                    concrete () = default;

                    // copy constructor
                    concrete (concrete const &copy) = default;

                    // stateful constructor
                    explicit concrete (state_type const &state) :
                        base_type {state} {}

                    // destructor
                    ~concrete () = default;
            };
        }
    }
}

#endif
