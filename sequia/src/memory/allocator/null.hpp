#ifndef _NULL_ALLOCATOR_HPP_
#define _NULL_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements null-operation semantics
            // Fulfills stateful allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <typename T>
            class null
            {
                public:
                    using value_type = T;
                    using state_type = base_state<T>;

                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    // rebind type
                    template <typename U> 
                    struct rebind { using other = null<U>; };

                public:
                    // default constructor
                    null () = default;

                    // stateful copy constructor
                    template <typename Allocator>
                    null (Allocator const &copy) :
                        null {copy.state()} {}

                    // stateful constructor
                    explicit null (state_type const &state) :
                        state_ {state} {}

                    // destructor
                    ~null () = default;

                public:
                    // return max allocation of zero
                    size_t max_size () const { return 0; }

                    // allocation is a null operation
                    value_type *allocate (size_t num, const void* = 0) {}

                    // deallocation is a null operation
                    void deallocate (T *ptr, size_t num) {}

                public:
                    state_type const &state() const { return state_; }

                private:
                    state_type  state_;
            };
        }
    }
}

#endif
