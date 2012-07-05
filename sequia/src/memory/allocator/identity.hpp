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

            template <typename Delegator>
            class identity : public Delegator
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
                        using other = identity
                            <typename Delegator::template rebind<U>::other>;
                    };

                public:
                    // default constructor
                    identity () = default;

                    // stateful copy constructor
                    template <typename Allocator>
                    identity (Allocator const &copy) :
                        identity {copy.state()} {}

                    // stateful constructor
                    explicit identity (state_type const &state) :
                        Delegator {state} {}

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
                    using Delegator::state;
            };
        }
    }
}

#endif
