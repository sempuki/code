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
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <size_t N, typename T, typename State = base_state<T>>
            class fixed_buffer
            {
                protected:
                    using base_type = std::false_type;

                public:
                    using value_type = T;
                    using state_type = State;

                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U>
                    struct rebind 
                    { 
                        using other = fixed_buffer<N, U>; 
                    };

                public:
                    // default constructor
                    fixed_buffer () : state_ {N} {}

                    // copy constructor
                    fixed_buffer (fixed_buffer const &copy) = delete;

                    // stateful constructor
                    explicit fixed_buffer (State const &state) :
                        state_ {state} 
                    {
                        buffer<T> &mem = state_.arena;
                        
                        ASSERTF (N == mem.size, "state allocation mismatch");
                    }

                    // destructor
                    ~fixed_buffer () = default;

                public:
                    size_t max_size () const 
                    { 
                        return N;
                    }

                    T *allocate (size_t num, const void* = 0) 
                    { 
                        buffer<T> &mem = state_.arena;
                        
                        ASSERTF (!mem.valid(), "previously allocated");
                        ASSERTF (N == num, "incorrect allocation size");

                        return mem.items = buffer_.items;
                    }

                    void deallocate (T *ptr, size_t num) 
                    {
                        buffer<T> &mem = state_.arena;
                        
                        ASSERTF (mem.valid(), "not previously allocated");
                        ASSERTF (mem.items == ptr, "not from this allocator");
                        ASSERTF (N == num, "incorrect allocation size");

                        mem.invalidate();
                    }

                public:
                    state_type const &state() const { return state_; }

                private:
                    static_buffer<T, N> buffer_;
                    state_type          state_;
            };
        }
    }
}

#endif
