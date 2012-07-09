#ifndef _IDENTITY_ALLOCATOR_HPP_
#define _IDENTITY_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=====================================================================
            // Only allocates the same memory arena for each call
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept

            template <typename Delegator,
                     typename Value = typename Delegator::value_type,
                     typename State = typename Delegator::state_type> 

            class identity : public Delegator
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
                        using other = identity
                            <typename base_type::template rebind<U>::other>;
                    };

                public:
                    // stateful copy constructor
                    template <typename Allocator>
                    identity (Allocator const &copy) :
                        identity {copy.state()} {}

                    // stateful constructor
                    explicit identity (State const &state) :
                        base_type {state} {}

                    // destructor
                    ~identity () = default;

                public:
                    // max available to allocate
                    size_t max_size () const 
                    { 
                        return state().arena.size;
                    }

                    // allocate number of items
                    value_type *allocate (size_t num, const void* = 0) 
                    { 
                        return state().arena.items; 
                    }

                    // deallocate number of items
                    void deallocate (value_type *ptr, size_t num) 
                    {
                    }
                
                public:
                    using base_type::state;
            };
        }
    }
}

#endif
