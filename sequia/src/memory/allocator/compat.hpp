#ifndef _COMPAT_ALLOCATOR_HPP_
#define _COMPAT_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements compatibility layer for old-style std allocators
            // Fulfills stateful allocator concept
            // Fulfills composable allocator concept

            namespace impl
            {
                template <typename Base, typename State, typename Type>
                class compat : public Base
                {
                    public:
                        using pointer = Type *;
                        using reference = Type &;
                        using const_pointer = Type const *;
                        using const_reference = Type const &;

                        using size_type = size_t;
                        using difference_type = ptrdiff_t;

                    public:
                        // default constructor
                        compat () :
                            Base {} {}

                        // copy constructor
                        compat (compat const &copy) :
                            Base {copy} {}

                        // stateful constructor
                        explicit compat (State const &state) :
                            Base {state} {}

                        // destructor
                        ~compat () {}

                    public:
                        pointer address (reference v) const { return &v; }
                        const_pointer address (const_reference v) const { return &v; }

                        template<typename... Args>
                        void construct (pointer p, Args&&... args)
                        {
                            new ((void *)p) Type (std::forward<Args>(args)...);
                        }

                        void destroy (pointer p)
                        {
                            p->~Type();
                        }
                };
            }

            template <typename Composite>
            struct compat
            {
                using base_type = Composite;
                
                template <typename T>
                using state_type = typename base_type::template state_type<T>;
                    
                template <typename S, typename T>
                using concrete_type = impl::compat<typename base_type::template concrete_type<S,T>, S, T>;
                    
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;
            };
        }
    }
}

#endif
