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
            // Fulfills terminal allocator concept

            template <typename ValueType, size_t N> 
            struct fixed_buffer
            {
                using base_type = terminal;
                using value_type = ValueType;
                using state_type = basic_state<value_type>;

                using propagate_on_container_copy_assignment = std::false_type;
                using propagate_on_container_move_assignment = std::false_type;
                using propagate_on_container_swap = std::false_type;
                    
                template <typename U>
                using rebind_type = fixed_buffer<U, N>;

                template <typename S, typename T>
                using concrete_type = impl::fixed_buffer<base_type::concrete_type, S, T, N>;
            };

            namespace impl
            {
                template <typename Base, typename State, typename Value, size_t N>
                class fixed_buffer : public Base <State, Value>
                {
                    public:
                        // default constructor
                        fixed_buffer ()
                        {
                            buffer<Value> &mem = Base::access_state().arena;
                            mem.size = N;

                            std::cout << "fixed default: sizeof(Value) " << std::dec << sizeof(Value) << std::endl;
                            std::cout << "fixed default: mem.items: " << std::hex << mem.items << std::endl;
                            std::cout << "fixed default: mem.size: " << std::dec << mem.size << std::endl;
                        }

                        // copy constructor
                        fixed_buffer (fixed_buffer const &copy) = delete;

                        // destructor
                        ~fixed_buffer () = default;

                        // stateful constructor
                        explicit fixed_buffer (State const &state) : 
                            Base {state}
                        {
                            ASSERTF (N == mem.size, "state allocation mismatch");

                            buffer<Value> &mem = Base::access_state().arena;

                            std::cout << "fixed state: sizeof(Value) " << std::dec << sizeof(Value) << std::endl;
                            std::cout << "fixed state: mem.items: " << std::hex << mem.items << std::endl;
                            std::cout << "fixed state: mem.size: " << std::dec << mem.size << std::endl;
                        }

                    public:
                        size_t max_size () const 
                        { 
                            return N;
                        }

                        Value *allocate (size_t num, const void* = 0) 
                        { 
                            buffer<Value> &mem = Base::access_state().arena;

                            ASSERTF (!mem.valid(), "previously allocated");
                            ASSERTF (N == num, "incorrect allocation size");

                            return mem.items = buffer_.items;
                        }

                        void deallocate (Value *ptr, size_t num) 
                        {
                            buffer<Value> &mem = Base::access_state().arena;

                            ASSERTF (mem.valid(), "not previously allocated");
                            ASSERTF (mem.items == ptr, "not from this allocator");
                            ASSERTF (N == num, "incorrect allocation size");

                            mem.invalidate();
                        }

                    private:
                        static_buffer<Value, N> buffer_;
                };
            }
        }
    }
}

#endif
