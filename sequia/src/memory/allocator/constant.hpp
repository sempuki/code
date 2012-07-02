#ifndef _SCOPED_ALLOCATOR_HPP_
#define _SCOPED_ALLOCATOR_HPP_

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
                    typedef typename Delegator::state_type  state_type;
                    typedef typename Delegator::value_type  value_type;

                    typedef std::true_type propagate_on_container_copy_assignment;
                    typedef std::true_type propagate_on_container_move_assignment;
                    typedef std::true_type propagate_on_container_swap;

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
