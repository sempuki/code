#ifndef _UNITY_ALLOCATOR_HPP_
#define _UNITY_ALLOCATOR_HPP_

namespace sequia
{
    namespace memory
    {
        namespace allocator
        {
            //=====================================================================
            // Only allocates single element per call
            // * Uses an embedded index-based linked free list
            // Fulfills stateful allocator concept
            // Fulfills composable allocator concept

            template <typename Composite, typename Index>
            struct unity
            {
                using base_type = Composite;

                struct value_type
                {
                    using index_type = Index;
                    using value_type = base_type::value_type;

                    union
                    {
                        index_type  index;
                        value_type  value;
                    };

                    value_type() : index {0} {}
                };
                    
                struct state_type : 
                    base_type::state_type<value_type>
                {
                    value_type  *head;

                    state_type () :
                        head {nullptr} {}

                    state_type (state_type const &copy) :
                        base_type::state_type<block_type<T>> {copy}, 
                        head {copy.head} {}
                };

                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;

                template <typename U>
                using rebind_type = unity<base_type::rebind_type<U>>;

                template <typename S, typename T>
                using concrete_type = impl::unity<base_type::concrete_type, S, T>;
            };

            namespace impl
            {
                template <typename Base, typename State, typename Value>
                class unity : public Base <State, Value>
                {
                    // TODO: Value == block_type, not T
                    public:
                        // default constructor
                        unity () = default;

                        // copy constructor
                        unity (unity const &copy) = default;

                        // destructor
                        ~unity () = default;

                        // state constructor
                        explicit unity (State const &state) :
                            Base {state}
                        {
                            block_type *&free = Base::access_state().head;
                            buffer<block_type> const &mem = Base::access_state().arena;

                            ASSERTF (mem.items != nullptr, "memory not allocated");
                            ASSERTF (mem.size < (core::one << (sizeof(Index) * 8)), 
                                    "too many objects for size of free list index type");

                            std::cout << "unity const: sizeof(block_type) " << std::dec << sizeof(block_type) << std::endl;
                            std::cout << "unity const: mem.items: " << std::hex << Base::access_state().arena.items << std::endl;
                            std::cout << "unity const: mem.size: " << std::dec << Base::access_state().arena.size << std::endl;

                            free = mem.items;

                            Index index = 0;
                            block_type *block = mem.items;
                            block_type *end   = mem.items + mem.size;

                            for (; block < end; ++block)
                                block->index = ++index;
                        }

                    public:
                        // allocate
                        Value *allocate (size_t num, const void* = 0)
                        {
                            block_type *&free = Base::access_state().head;
                            buffer<block_type> const &mem = Base::access_state().arena;

                            ASSERTF (num == 1, "can only allocate one object per call");
                            ASSERTF (mem.contains (free), "free list is corrupt");

                            std::cout << "alloc: sizeof(block_type) " << std::dec << sizeof(block_type) << std::endl;
                            std::cout << "alloc: mem.items: " << std::hex << mem.items << std::endl;
                            std::cout << "alloc: mem.size: " << std::dec << mem.size << std::endl;

                            block_type *block = free;
                            free = mem.items + free->index;

                            return reinterpret_cast <Value *> (block);
                        }

                        // deallocate
                        void deallocate (Value *ptr, size_t num)
                        {
                            block_type *&free = Base::access_state().head;
                            buffer<block_type> const &mem = Base::access_state().arena;

                            ASSERTF (num == 1, "can only allocate one object per call");
                            ASSERTF (mem.contains (ptr), "pointer is not from this heap");

                            block_type *block = reinterpret_cast <block_type *> (ptr);

                            block->index = free - mem.items;
                            free = block;
                        }
                };
            }
        }
    }
}


#endif
