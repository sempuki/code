#ifndef _NULL_ALLOCATOR_HPP_
#define _NULL_ALLOCATOR_HPP_

namespace ceres
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements null-operation semantics
            // Fulfills stateful allocator concept
            // Fulfills composable allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <typename ConcreteValue = std::false_type, 
                     typename ConcreteState = std::false_type>

            class null :
                public terminal<ConcreteState>
            {
                public:
                    using base_type = terminal<std::false_type>;
                    using state_type = std::false_type;
                    using value_type = ConcreteValue;

                public:
                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U> 
                    struct rebind { using other = null<U>; };

                    template <typename U, typename S>
                    struct reify { using other = null<U, S>; };

                public:
                    // default constructor
                    null () = default;

                    // copy constructor
                    null (null const &copy) = default;

                    // stateful constructor
                    explicit null (state_type const &state) {}

                    // destructor
                    ~null () = default;

                public:
                    // return max allocation of zero
                    size_t max_size () const { return 0; }

                    // allocation is a null operation
                    T *allocate (size_t num, const void* = 0) {}

                    // deallocation is a null operation
                    void deallocate (T *ptr, size_t num) {}
            };
        }
    }
}

#endif
