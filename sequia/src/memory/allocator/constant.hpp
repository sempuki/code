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
            // Fulfills stateful allocator Concept

            template <size_t N, typename Delegator>
            class constant : public Delegator
            {
                public:
                    using value_type = typename Delegator::value_type;
                    using state_type = typename Delegator::state_type;

                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    // default (stateful) constructor
                    constant () : Delegator {state_type {N}} {} 

                    // stateful copy constructor
                    template <typename Allocator>
                    constant (Allocator const &copy) :
                        constant {copy.state()} {}

                    // destructor
                    ~constant () = default;

                public:
                    using Delegator::state;
            };
        }
    }
}

#endif
