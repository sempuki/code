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

            namespace impl
            {
                template <typename Base, typename State, typename Type>
                class identity : public Base
                {
                    public:
                        // default constructor
                        identity () :
                            Base {} {}

                        // copy constructor
                        identity (identity const &copy) :
                            Base {copy} {}

                        // stateful constructor
                        explicit identity (State const &state) :
                            Base {state} {}

                        // destructor
                        ~identity () {}

                    public:
                        // max available to allocate
                        size_t max_size () const 
                        { 
                            return Base::access_state().arena.size;
                        }

                        // allocate number of items
                        Type *allocate (size_t num, const void* = 0) 
                        { 
                            return Base::access_state().arena.items; 
                        }

                        // deallocate number of items
                        void deallocate (Type *ptr, size_t num) 
                        {
                        }
                };
            }

            template <typename Composite>
            struct identity
            {
                using base_type = Composite;
                
                template <typename T>
                using state_type = typename base_type::template state_type<T>;

                template <typename S, typename T>
                using concrete_type = impl::identity<typename base_type::template concrete_type<S,T>, S, T>;
                    
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;
            };
        }
    }
}

#endif
