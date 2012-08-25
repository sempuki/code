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

            template <typename Composite>
            struct identity
            {
                using base_type = Composite;
                using value_type = base_type::value_type;
                using state_type = base_type::state_type<value_type>;
                    
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;

                template <typename U>
                using rebind_type = identity<base_type::rebind_type<U>>;

                template <typename S, typename T>
                using concrete_type = impl::identity<base_type::concrete_type, S, T>;
            };

            namespace impl
            {
                template <typename Base, typename State, typename Value>
                class identity : public Base <State, Value>
                {
                    public:
                        // default constructor
                        identity () = default;

                        // copy constructor
                        identity (identity const &copy) = default;

                        // stateful constructor
                        explicit identity (State const &state) :
                            Base {state} {}

                        // destructor
                        ~identity () = default;

                    public:
                        // max available to allocate
                        size_t max_size () const 
                        { 
                            return Base::access_state().arena.size;
                        }

                        // allocate number of items
                        Value *allocate (size_t num, const void* = 0) 
                        { 
                            return Base::access_state().arena.items; 
                        }

                        // deallocate number of items
                        void deallocate (Value *ptr, size_t num) 
                        {
                        }
                };
            }
        }
    }
}

#endif
