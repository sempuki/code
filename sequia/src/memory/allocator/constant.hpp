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

            template <size_t N, 
                     typename Delegator, 
                     typename Value = typename Delegator::value_type,
                     typename State = typename Delegator::state_type> 

            class constant : public Delegator
            {
                protected:
                    using base_type = Delegator;

                public:
                    using value_type = Value;
                    using state_type = State;

                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U> 
                    struct rebind 
                    { 
                        using other = constant
                            <N, typename base_type::template rebind<U>::other>;
                    };

                public:
                    // default (stateful) constructor
                    constant () : base_type {state_type {N}} {} 

                    // stateful copy constructor
                    template <typename Allocator>
                    constant (Allocator const &copy) :
                        constant {copy.state()} {}

                    // destructor
                    ~constant () = default;

                public:
                    using base_type::state;
            };
        }
    }
}

#endif
