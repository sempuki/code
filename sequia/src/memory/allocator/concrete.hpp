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
            // Fulfills concrete allocator concept
            
            namespace details
            {
                template <typename Base>
                using concrete_base = 
                    Base::concrete_type<typename Base::state_type, typename Base::value_type>;
            }

            template <typename Composite>
            class concrete : public details::concrete_base<Composite>
            {
                public:
                    using base_type = details::concrete_base<Composite>;
                    using state_type = typename Composite::state_type;
                    using value_type = typename Composite::value_type;

                public:
                    using propagate_on_container_copy_assignment = typename Composite::propagate_on_container_copy_assignment;
                    using propagate_on_container_move_assignment = typename Composite::propagate_on_container_move_assignment;
                    using propagate_on_container_swap = typename Composite::propagate_on_container_swap;

                public:
                    template <typename U> 
                    struct rebind { using other = concrete<Composite::rebind_type<U>>; };

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
