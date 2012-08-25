#ifndef _COMPAT_ALLOCATOR_HPP_
#define _COMPAT_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements scoped allocate-on-construction semantics
            // Fulfills stateful allocator concept
            // Fulfills composable allocator concept
            // Fulfills rebindable allocator concept

            template <typename Delegator, 
                     typename ConcreteValue = std::false_type,
                     typename ConcreteState = std::false_type>

            class compat : 
                public detail::base<Delegator, ConcreteValue, ConcreteState>
            {
                public:
                    using base_type = detail::base<Delegator, ConcreteValue, ConcreteState>;
                    using value_type = ConcreteValue;

                    struct state_type : 
                        base_type::state_type {};

                public:
                    using pointer = value_type *;
                    using reference = value_type &;
                    using const_pointer = value_type const *;
                    using const_reference = value_type const &;

                    using size_type = size_t;
                    using difference_type = ptrdiff_t;

                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U>
                    struct rebind { using other = compat<Delegator, U>; };

                   template <typename U, typename S> 
                    struct reify { using other = compat<Delegator, U, S>; };

                public:
                    // default constructor
                    compat () = default;

                    // copy constructor
                    compat (compat const &copy) = default;

                    // stateful constructor
                    explicit compat (state_type const &state) :
                        base_type {state} {}

                    // destructor
                    ~compat () = default;

                public:
                    pointer address (reference v) const { return &v; }
                    const_pointer address (const_reference v) const { return &v; }

                    template<typename... Args>
                    void construct (pointer p, Args&&... args)
                    {
                        new ((void *)p) value_type (std::forward<Args>(args)...);
                    }

                    void destroy (pointer p)
                    {
                        p->~value_type();
                    }
            };
        }
    }
}

#endif
