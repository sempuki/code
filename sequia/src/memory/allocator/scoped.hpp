#ifndef _SCOPED_ALLOCATOR_HPP_
#define _SCOPED_ALLOCATOR_HPP_

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

            template <typename Composite>
            struct scoped
            {
                using base_type = Composite;
                using value_type = base_type::value_type;
                using state_type = base_type::state_type<value_type>;
                    
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;

                template <typename U>
                using rebind_type = scoped<base_type::rebind_type<U>>;

                template <typename S, typename T>
                using concrete_type = impl::scoped<base_type::concrete_type, S, T>;
            };

            namespace impl
            {
                template <typename Base, typename State, typename Value>
                class scoped : public Base <State, Value>
                {
                    public:
                        // default constructor
                        scoped ()
                        {
                            buffer<Value> &mem = Base::access_state().arena;

                            mem.items = Base::allocate (mem.size);

                            std::cout << "scoped default: sizeof(Value) " << std::dec << sizeof(Value) << std::endl;
                            std::cout << "scoped default: mem.items: " << std::hex << mem.items << std::endl;
                            std::cout << "scoped default: mem.size: " << std::dec << mem.size << std::endl;
                        }    

                        // copy constructor
                        scoped (scoped const &copy)
                        {
                            buffer<Value> &mem = Base::access_state().arena;

                            mem.items = Base::allocate (mem.size);
                            
                            std::cout << "scoped copy: sizeof(Value) " << std::dec << sizeof(Value) << std::endl;
                            std::cout << "scoped copy: mem.items: " << std::hex << mem.items << std::endl;
                            std::cout << "scoped copy: mem.size: " << std::dec << mem.size << std::endl;
                        }

                        // stateful constructor
                        explicit scoped (State const &state) :
                            Base {state}
                        {
                            buffer<Value> &mem = Base::access_state().arena;

                            mem.items = Base::allocate (mem.size);
                            
                            std::cout << "scoped state: sizeof(Value) " << std::dec << sizeof(Value) << std::endl;
                            std::cout << "scoped state: mem.items: " << std::hex << mem.items << std::endl;
                            std::cout << "scoped state: mem.size: " << std::dec << mem.size << std::endl;
                        }

                        // destructor
                        ~scoped ()
                        {
                            buffer<Value> &mem = Base::access_state().arena;

                            Base::deallocate (mem.items, mem.size);
                        }
                };
            }
        }
    }
}

#endif
