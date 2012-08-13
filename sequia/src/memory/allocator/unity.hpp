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
            // Fulfills rebindable allocator concept

            namespace detail
            {
                template <typename Index, typename Value>
                union block
                {
                    using index_type = Index;
                    using value_type = Value;

                    index_type  index;
                    value_type  value;

                    block() : index {0} {}
                };


                template <typename Delegator, 
                         typename Index,
                         typename Value = typename Delegator::value_type> 

                using base = typename Delegator::template rebind<block<Index, Value>>::other;
            }

            //-----------------------------------------------------------------

            template <typename Delegator, typename Index, typename T = void>
            class unity : public detail::base<Delegator, Index>
            {
                public:
                    using base_type = detail::base<Delegator, Index>;

                    using value_type = T;
                    using block_type = detail::block<Index, T>;

                    struct state_type : typename base_type::state_type
                    {
                        block_type  *head;

                        state (state const &copy) :
                            base {copy.base}, head {copy.head} {}
                    };

                public:
                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U>
                    struct rebind 
                    { 
                        using other = unity 
                            <typename base_type::template rebind<U>::other, Index, T>;
                    };
                
                public:
                    // default constructor
                    unity () = default;

                    // copy constructor
                    unity (unity const &copy) = default;

                    // destructor
                    ~unity () = default;

                    // state constructor
                    explicit unity (state_type const &state) :
                        base_type {state.base}, state_ {base_type::state()} 
                    {
                        block_type *&free = state().head;
                        buffer<block_type> const &mem = state().base->arena;

                        ASSERTF (mem.items != nullptr, "memory not allocated");
                        ASSERTF (mem.size < (core::one << (sizeof(Index) * 8)), 
                                "too many objects for size of free list index type");

                        std::cout << "unity const: sizeof(block_type) " << std::dec << sizeof(block_type) << std::endl;
                        std::cout << "unity const: mem.items: " << std::hex << state().base->arena.items << std::endl;
                        std::cout << "unity const: mem.size: " << std::dec << state().base->arena.size << std::endl;
                        std::cout << "!! " << &(unity::state()) << " : " << state().base << std::endl;

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
                        block_type *&free = state().head;
                        buffer<block_type> const &mem = state().base->arena;

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
                        block_type *&free = state().head;
                        buffer<block_type> const &mem = state().base->arena;

                        ASSERTF (num == 1, "can only allocate one object per call");
                        ASSERTF (mem.contains (ptr), "pointer is not from this heap");

                        block_type *block = reinterpret_cast <block_type *> (ptr);

                        block->index = free - mem.items;
                        free = block;
                    }

                public:
                    // state accessor
                    virtual state_type &state() = 0;
            };
        }
    }
}


#endif
