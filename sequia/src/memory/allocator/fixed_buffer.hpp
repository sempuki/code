#ifndef _FIXED_ALLOCATOR_HPP_
#define _FIXED_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=========================================================================
            // Implements fixed-buffer semantics
            // Fulfills stateful allocator concept
            // Fulfills terminal allocator concept

            template <typename T, size_t N>
            class fixed_buffer
            {
                public:
                    using value_type = T;
                    using state_type = base_state<T>;

                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                    //using pointer = typename std::allocator_traits<value_type>::pointer;

                public:
                    // default constructor
                    fixed_buffer () :
                        state_ {N} {}

                    // copy constructor
                    fixed_buffer (fixed_buffer const &copy) = delete;

                    // stateful constructor
                    explicit fixed_buffer (state_type const &state) :
                        state_ {state} 
                    {
                        buffer<value_type> &mem = state_.arena;
                        
                        ASSERTF (N == mem.size, "state allocation mismatch");
                    }

                    // destructor
                    ~fixed_buffer () = default;

                public:
                    size_t max_size () const 
                    { 
                        return N;
                    }

                    value_type *allocate (size_t num, const void* = 0) 
                    { 
                        buffer<value_type> &mem = state_.arena;
                        
                        ASSERTF (!mem.valid(), "previously allocated");
                        ASSERTF (N == num, "incorrect allocation size");

                        return mem.items = buffer_.items;
                    }

                    void deallocate (value_type *ptr, size_t num) 
                    {
                        buffer<value_type> &mem = state_.arena;
                        
                        ASSERTF (mem.valid(), "not previously allocated");
                        ASSERTF (mem.items == ptr, "not from this allocator");
                        ASSERTF (N == num, "incorrect allocation size");

                        mem.invalidate();
                    }

                public:
                    state_type const &state() const { return state_; }

                private:
                    static_buffer<value_type, N>    buffer_;
                    state_type                      state_;
            };
        }
    }
}

#endif
