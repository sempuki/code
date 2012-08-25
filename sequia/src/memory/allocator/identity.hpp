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
            // Fulfills composable allocator concept
            // Fulfills rebindable allocator concept

            template <typename Delegator, 
                     typename ConcreteValue = std::false_type,
                     typename ConcreteState = std::false_type>

            class identity : 
                public detail::base<Delegator, ConcreteValue, ConcreteState>
            {
                public:
                    using base_type = detail::base<Delegator, ConcreteValue, ConcreteState>;
                    using value_type = ConcreteValue;

                    struct state_type : 
                        base_type::state_type {};

                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U>
                    struct rebind { using other = identity<Delegator, U>; };

                    template <typename U, typename S> 
                    struct reify { using other = identity<Delegator, U, S>; };

                public:
                    // default constructor
                    identity () = default;

                    // copy constructor
                    identity (identity const &copy) = default;

                    // stateful constructor
                    explicit identity (state_type const &state) :
                        base_type {state} {}

                    // destructor
                    ~identity () = default;

                public:
                    // max available to allocate
                    size_t max_size () const 
                    { 
                        return base_type::access_state().arena.size;
                    }

                    // allocate number of items
                    value_type *allocate (size_t num, const void* = 0) 
                    { 
                        return base_type::access_state().arena.items; 
                    }

                    // deallocate number of items
                    void deallocate (value_type *ptr, size_t num) 
                    {
                    }
            };
        }
    }
}

#endif
