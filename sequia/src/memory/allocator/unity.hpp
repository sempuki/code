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

            namespace indexed
            {
                template <typename Block, typename State>
                struct state : public State
                {
                    State const &base;
                    Block       *head;

                    state (State const &copy) :
                        base {copy}, head {nullptr} {}

                    state (state const &copy) :
                        base {copy.base}, head {copy.head} {}
                };


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

            template <typename Delegator,
                     typename Index, 
                     typename Value = typename Delegator::value_type,
                     typename State = typename Delegator::state_type> 

            class unity : public indexed::base<Delegator, Index>
            {
                protected:
                    using base_type = indexed::base<Delegator, Index>;
                    using block_type = typename base_type::value_type;
                    using base_state = typename base_type::state_type;

                public:
                    using value_type = Value;
                    using state_type = indexed::state<block_type, base_state>;

                    using propagate_on_container_copy_assignment = std::true_type;
                    using propagate_on_container_move_assignment = std::true_type;
                    using propagate_on_container_swap = std::true_type;

                public:
                    template <typename U>
                    struct rebind 
                    { 
                        using other = unity 
                            <typename base_type::template rebind<U>::other, Index>;
                    };

                public:
                    // stateful copy constructor
                    template <typename Allocator>
                    unity (Allocator const &copy) :
                        unity {copy.state()} {}

                    // indexed state constructor
                    explicit unity (state_type const &state) :
                        unity {static_cast <base_state const &> (state)} {}

                    // base state constructor
                    explicit unity (base_state const &state) :
                        base_type {state}, state_ {base_type::state()} 
                    {
                        block_type *&free = state_.head;
                        buffer<block_type> const &mem = state_.base.arena;

                        ASSERTF (mem.items != nullptr, "memory not allocated");
                        ASSERTF (mem.size < (core::one << (sizeof(Index) * 8)), 
                                "too many objects for size of free list index type");

                        free = mem.items;

                        Index index = 0;
                        block_type *block = mem.items;
                        block_type *end   = mem.items + mem.size;

                        for (; block < end; ++block)
                            block->index = index++;
                    }

                    // destructor
                    ~unity () = default;

                public:
                    // allocate
                    value_type *allocate (size_t num, const void* = 0)
                    {
                        block_type *&free = state_.head;
                        buffer<block_type> const &mem = state_.base.arena;

                        ASSERTF (num == 1, "can only allocate one object per call");
                        ASSERTF (mem.contains (free), "free list is corrupt");

                        block_type *block = free;
                        free = mem.items + free->index;

                        return reinterpret_cast <value_type *> (block);
                    }

                    // deallocate
                    void deallocate (value_type *ptr, size_t num)
                    {
                        block_type *&free = state_.head;
                        buffer<block_type> const &mem = state_.base.arena;

                        ASSERTF (num == 1, "can only allocate one object per call");
                        ASSERTF (mem.contains (ptr), "pointer is not from this heap");

                        block_type *block = reinterpret_cast <block_type *> (ptr);

                        block->index = free - mem.items;
                        free = block;
                    }

                public:
                    state_type const &state() const { return state_; }

                private:
                    state_type  state_;
            };
        }
    }
}


#endif
