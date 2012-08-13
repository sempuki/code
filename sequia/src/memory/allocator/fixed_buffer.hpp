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
            // Fulfills composable allocator concept
            // Fulfills rebindable allocator concept
            // Fulfills terminal allocator concept

            template <size_t N, 
                     typename ConcreteValue = std::false_type, 
                     typename ConcreteState = std::false_type>

            class fixed_buffer :
                public terminal<ConcreteState>
            {
                public:
                    using base_type = terminal<ConcreteState>;
                    using state_type = basic_state<ConcreteValue>;
                    using value_type = ConcreteValue;
                    
                public:
                    using propagate_on_container_copy_assignment = std::false_type;
                    using propagate_on_container_move_assignment = std::false_type;
                    using propagate_on_container_swap = std::false_type;

                public:
                    template <typename U>
                    struct rebind { using other = fixed_buffer<N, U>; };

                    template <typename U, typename S>
                    struct reify { using other = fixed_buffer<N, U, S>; };

                public:
                    // default constructor
                    fixed_buffer ()
                    {
                        buffer<value_type> &mem = base_type::access_state().arena;
                        mem = buffer_;
                        
                        std::cout << "fixed default: sizeof(value_type) " << std::dec << sizeof(value_type) << std::endl;
                        std::cout << "fixed default: mem.items: " << std::hex << mem.items << std::endl;
                        std::cout << "fixed default: mem.size: " << std::dec << mem.size << std::endl;
                    }

                    // copy constructor
                    fixed_buffer (fixed_buffer const &copy) = delete;

                    // destructor
                    ~fixed_buffer () = default;

                    // stateful constructor
                    explicit fixed_buffer (state_type const &state)
                    {
                        ASSERTF (N == mem.size, "state allocation mismatch");

                        buffer<value_type> &mem = base_type::access_state().arena;
                        
                        std::cout << "fixed state: sizeof(value_type) " << std::dec << sizeof(value_type) << std::endl;
                        std::cout << "fixed state: mem.items: " << std::hex << mem.items << std::endl;
                        std::cout << "fixed state: mem.size: " << std::dec << mem.size << std::endl;
                    }

                public:
                    size_t max_size () const 
                    { 
                        return N;
                    }

                    value_type *allocate (size_t num, const void* = 0) 
                    { 
                        buffer<value_type> &mem = base_type::access_state().arena;
                        
                        ASSERTF (!mem.valid(), "previously allocated");
                        ASSERTF (N == num, "incorrect allocation size");

                        return mem.items = buffer_.items;
                    }

                    void deallocate (value_type *ptr, size_t num) 
                    {
                        buffer<value_type> &mem = base_type::access_state().arena;
                        
                        ASSERTF (mem.valid(), "not previously allocated");
                        ASSERTF (mem.items == ptr, "not from this allocator");
                        ASSERTF (N == num, "incorrect allocation size");

                        mem.invalidate();
                    }

                private:
                    static_buffer<value_type, N>    buffer_;
            };
        }
    }
}

#endif
