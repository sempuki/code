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
            // Fulfills stateful allocator Concept

            template <typename Delegator>
            class identity : public Delegator
            {
                public:
                    typedef typename Delegator::state_type  state_type;
                    typedef typename Delegator::value_type  value_type;

                    typedef std::true_type propagate_on_container_copy_assignment;
                    typedef std::true_type propagate_on_container_move_assignment;
                    typedef std::true_type propagate_on_container_swap;

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
