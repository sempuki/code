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

            template <typename Composite>
            struct compat
            {
                using base_type = Composite;
                using value_type = base_type::value_type;
                using state_type = base_type::state_type<value_type>;
                    
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;

                template <typename U>
                using rebind_type = compat<base_type::rebind_type<U>>;

                template <typename S, typename T>
                using concrete_type = impl::compat<base_type::concrete_type<S, T>, S, T>;
            };

            namespace impl
            {
                template <typename Base, typename State, typename Value>
                class compat : public Base
                {
                    public:
                        // default constructor
                        compat () = default;

                        // copy constructor
                        compat (compat const &copy) = default;

                        // stateful constructor
                        explicit compat (State const &state) :
                            Base {state} {}

                        // destructor
                        ~compat () = default;

                    public:
                        pointer address (reference v) const { return &v; }
                        const_pointer address (const_reference v) const { return &v; }

                        template<typename... Args>
                        void construct (pointer p, Args&&... args)
                        {
                            new ((void *)p) Value (std::forward<Args>(args)...);
                        }

                        void destroy (pointer p)
                        {
                            p->~Value();
                        }
                };
            }
        }
    }
}

#endif
