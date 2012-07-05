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

            template <typename Delegator>
            class scoped : public Delegator
            {
                public:
                    using value_type = typename Delegator::value_type;
                    using state_type = typename Delegator::state_type;

                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    // rebind type
                    template <typename U> 
                    struct rebind 
                    { 
                        using other = scoped
                            <typename Delegator::template rebind<U>::other>;
                    };

                public:
                    // default constructor
                    scoped () = delete;

                    // stateful copy constructor
                    template <typename Allocator>
                    scoped (Allocator const &copy) :
                        scoped {copy.state()} {}

                    // stateful constructor
                    explicit scoped (state_type const &state) :
                        Delegator {state}, state_ {state}
                    {
                        buffer<value_type> &mem = state_.arena;

                        mem.items = Delegator::allocate (mem.size);
                    }

                    // destructor
                    ~scoped ()
                    {
                        buffer<value_type> &mem = state_.arena;

                        Delegator::deallocate (mem.items, mem.size);
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
