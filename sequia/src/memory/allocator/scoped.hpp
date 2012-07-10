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
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept

            template <typename Delegator,
                     typename Value = typename Delegator::value_type,
                     typename State = typename Delegator::state_type> 

            class scoped : public Delegator
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
                        using other = scoped
                            <typename base_type::template rebind<U>::other>;
                    };

                public:
                    // stateful copy constructor
                    template <typename Allocator>
                    scoped (Allocator const &copy) :
                        scoped {copy.state()} {}

                    // stateful constructor
                    explicit scoped (state_type const &state) :
                        base_type {state}, state_ {base_type::state()}
                    {
                        buffer<value_type> &mem = state_.arena;

                        mem.items = base_type::allocate (mem.size);
                    }

                    // destructor
                    ~scoped ()
                    {
                        buffer<value_type> &mem = state_.arena;

                        base_type::deallocate (mem.items, mem.size);
                    }

                public:
                    state_type const &state() const { return state_; }

                private:
                    state_type  state_;
            };
        }
    }
}

#endif
