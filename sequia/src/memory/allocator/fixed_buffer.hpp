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

            namespace impl
            {
                template <typename Base, typename State, typename Type, size_t N>
                class fixed_buffer : public Base
                {
                    public:
                        // default constructor
                        fixed_buffer () :
                            Base {} { common_initialization (); }

                        // stateful constructor
                        explicit fixed_buffer (State const &state) : 
                            Base {state} { common_initialization (); }

                        // copy constructor
                        fixed_buffer (fixed_buffer const &copy) = delete;

                        // destructor
                        ~fixed_buffer () { common_finalization (); }

                    public:
                        size_t max_size () const 
                        { 
                            return N;
                        }

                        Type *allocate (size_t num, const void* = 0) 
                        { 
                            buffer<Type> &mem = Base::access_state().arena;

                            ASSERTF (!mem.valid(), "previously allocated");
                            ASSERTF (N == num, "incorrect allocation size");

                            return mem.items = buffer_.items;
                        }

                        void deallocate (Type *ptr, size_t num) 
                        {
                            buffer<Type> &mem = Base::access_state().arena;

                            ASSERTF (mem.valid(), "not previously allocated");
                            ASSERTF (mem.items == ptr, "not from this allocator");
                            ASSERTF (N == num, "incorrect allocation size");

                            mem.invalidate();
                        }
                        
                    private:
                        void common_initialization ()
                        {
                            buffer<Type> &mem = Base::access_state().arena;
                            mem.size = N;
                        }

                        void common_finalization ()
                        {
                            buffer<Type> &mem = Base::access_state().arena;
                            mem.size = 0;
                        }

                    private:
                        static_buffer<Type, N>  buffer_;
                };
            }

            template <size_t N> 
            struct fixed_buffer
            {
                template <typename T>
                using state_type = basic_state<T>;

                template <typename S, typename T>
                using concrete_type = impl::fixed_buffer <get_concrete_type <terminal, S, T>, S, T, N>;

                using propagate_on_container_copy_assignment = std::false_type;
                using propagate_on_container_move_assignment = std::false_type;
                using propagate_on_container_swap = std::false_type;
            };
        }
    }
}

#endif
