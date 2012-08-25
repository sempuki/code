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
            // Fulfills rebindable allocator concept

            template <typename Delegator, typename Index,
                     typename ConcreteValue = std::false_type,
                     typename ConcreteState = std::false_type>

            class unity : 
                public detail::base<Delegator, detail::block<Index,ConcreteValue>, ConcreteState>
            {
                public:
                    using base_type = detail::base<Delegator, detail::block<Index,ConcreteValue>, ConcreteState>;

                    using value_type = ConcreteValue;
                    using block_type = detail::block<Index,ConcreteValue>;

                    struct state_type : 
                        base_type::state_type
                    {
                        block_type  *head;

                        state_type () :
                            head {nullptr} {}

                        state_type (state_type const &copy) :
                            base_type::state_type {copy}, head {copy.head} {}
                    };

                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U>
                    struct rebind { using other = unity<Delegator, Index, U>; };

                    template <typename U, typename S> 
                    struct reify { using other = unity<Delegator, Index, U, S>; };

                public:
                    // default constructor
                    unity () = default;

                    // copy constructor
                    unity (unity const &copy) = default;

                    // destructor
                    ~unity () = default;

                    // state constructor
                    explicit unity (state_type const &state) :
                        base_type {state}
                    {
                        block_type *&free = base_type::access_state().head;
                        buffer<block_type> const &mem = base_type::access_state().arena;

                        ASSERTF (mem.items != nullptr, "memory not allocated");
                        ASSERTF (mem.size < (core::one << (sizeof(Index) * 8)), 
                                "too many objects for size of free list index type");

                        std::cout << "unity const: sizeof(block_type) " << std::dec << sizeof(block_type) << std::endl;
                        std::cout << "unity const: mem.items: " << std::hex << base_type::access_state().arena.items << std::endl;
                        std::cout << "unity const: mem.size: " << std::dec << base_type::access_state().arena.size << std::endl;

                        free = mem.items;

                        Index index = 0;
                        block_type *block = mem.items;
                        block_type *end   = mem.items + mem.size;

                        for (; block < end; ++block)
                            block->index = ++index;
                    }

                public:
                    // allocate
                    value_type *allocate (size_t num, const void* = 0)
                    {
                        block_type *&free = base_type::access_state().head;
                        buffer<block_type> const &mem = base_type::access_state().arena;

                        ASSERTF (num == 1, "can only allocate one object per call");
                        ASSERTF (mem.contains (free), "free list is corrupt");

                        std::cout << "alloc: sizeof(block_type) " << std::dec << sizeof(block_type) << std::endl;
                        std::cout << "alloc: mem.items: " << std::hex << mem.items << std::endl;
                        std::cout << "alloc: mem.size: " << std::dec << mem.size << std::endl;

                        block_type *block = free;
                        free = mem.items + free->index;

                        return reinterpret_cast <value_type *> (block);
                    }

                    // deallocate
                    void deallocate (value_type *ptr, size_t num)
                    {
                        block_type *&free = base_type::access_state().head;
                        buffer<block_type> const &mem = base_type::access_state().arena;

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


#endif
