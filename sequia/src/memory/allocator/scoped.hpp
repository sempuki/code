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
            // Fulfills rebindable allocator concept

            template <typename Delegator, 
                     typename ConcreteValue = std::false_type,
                     typename ConcreteState = std::false_type>

            class scoped : 
                public detail::base<Delegator, ConcreteValue, ConcreteState>
            {
                public:
                    using base_type = detail::base<Delegator, ConcreteValue, ConcreteState>;
                    using value_type = ConcreteValue;

                    struct state_type : 
                        base_type::state_type {};

                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U>
                    struct rebind { using other = scoped<Delegator, U>; };

                    template <typename U, typename S> 
                    struct reify { using other = scoped<Delegator, U, S>; };

                public:
                    // default constructor
                    scoped ()
                    {
                        buffer<value_type> &mem = base_type::access_state().arena;

                        mem.items = base_type::allocate (mem.size);

                        std::cout << "scoped default: sizeof(value_type) " << std::dec << sizeof(value_type) << std::endl;
                        std::cout << "scoped default: mem.items: " << std::hex << mem.items << std::endl;
                        std::cout << "scoped default: mem.size: " << std::dec << mem.size << std::endl;
                    }    

                    // copy constructor
                    scoped (scoped const &copy)
                    {
                        buffer<value_type> &mem = base_type::access_state().arena;

                        mem.items = base_type::allocate (mem.size);
                        
                        std::cout << "scoped copy: sizeof(value_type) " << std::dec << sizeof(value_type) << std::endl;
                        std::cout << "scoped copy: mem.items: " << std::hex << mem.items << std::endl;
                        std::cout << "scoped copy: mem.size: " << std::dec << mem.size << std::endl;
                    }

                    // stateful constructor
                    explicit scoped (state_type const &state) :
                        base_type {state}
                    {
                        buffer<value_type> &mem = base_type::access_state().arena;

                        mem.items = base_type::allocate (mem.size);
                        
                        std::cout << "scoped state: sizeof(value_type) " << std::dec << sizeof(value_type) << std::endl;
                        std::cout << "scoped state: mem.items: " << std::hex << mem.items << std::endl;
                        std::cout << "scoped state: mem.size: " << std::dec << mem.size << std::endl;
                    }

                    // destructor
                    ~scoped ()
                    {
                        buffer<value_type> &mem = base_type::access_state().arena;

                        base_type::deallocate (mem.items, mem.size);
                    }
            };
        }
    }
}

#endif
