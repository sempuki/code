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
            
            template <typename Composite, typename ConcreteType>
            class concrete : public get_concrete_base_type <Composite, ConcreteType>
            {
                public:
                    using base_type = get_concrete_base_type <Composite, ConcreteType>;
                    using state_type = get_state_type <Composite, ConcreteType>;
                    using value_type = ConcreteType;

                public:
                    using propagate_on_container_copy_assignment = typename Composite::propagate_on_container_copy_assignment;
                    using propagate_on_container_move_assignment = typename Composite::propagate_on_container_move_assignment;
                    using propagate_on_container_swap = typename Composite::propagate_on_container_swap;

                public:
                    template <typename U> 
                    struct rebind { using other = concrete<Composite, U>; };

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
