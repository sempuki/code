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
            
            /* To construct a composable allocator: 
             *  1. create a templated class that describes:
             *   a. How the to extend the state object exposed by the base allocator
             *   b. How to create a concrete allocator from Base, State, and T types
             *  2. implement a templated class that intercepts or redirects allocator calls
             *  3. compose allocators starting from concrete<> and ending with terminal<>
             *
             *  The State class is effectively the interface from base classes to parent
             *  to allow introspection into the functioning of the composite allocator.
             *  Extending classes should derive state_type to add new features, and 
             *  wrap value types when passing concrete_type to the base class; ie. 
             *  state_type is modified when passed to concrete<>, where it is passed
             *  unchanged to terminal<>; value_type is modified as it is passed down to 
             *  base classes, not on the way up to concrete<>.
             */

            template <typename Composite, typename ConcreteType>
            class concrete : public get_concrete_base_type <Composite, ConcreteType>
            {
                public:
                    using base_type = get_concrete_base_type <Composite, ConcreteType>;
                    using state_type = get_state_type <Composite, ConcreteType>;
                    using value_type = ConcreteType;

                public:
                    using propagate_on_container_copy_assignment = 
                        typename Composite::propagate_on_container_copy_assignment;

                    using propagate_on_container_move_assignment = 
                        typename Composite::propagate_on_container_move_assignment;

                    using propagate_on_container_swap = 
                        typename Composite::propagate_on_container_swap;

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
