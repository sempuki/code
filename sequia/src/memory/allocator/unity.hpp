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

            namespace impl
            {
                template <typename Index, typename Type>
                union block
                {
                    using index_type = Index;
                    using value_type = Type;

                    index_type  index;
                    value_type  value;

                    block() : index {0} {}
                };

                template <typename Base, typename State, typename Type, typename Index>
                class unity : public Base
                {
                    private:
                        using block_type = block<Index,Type>;

                    public:
                        // default constructor
                        unity () :
                            Base {} { common_initialization(); }

                        // copy constructor
                        unity (unity const &copy) :
                            Base {copy} { common_initialization(); }

                        // state constructor
                        explicit unity (State const &state) :
                            Base {state} { common_initialization(); }

                        // destructor
                        ~unity () { common_finalization (); }

                    public:
                        // allocate
                        Type *allocate (size_t num, const void* = 0)
                        {
                            block_type *&free = Base::access_state().head;
                            buffer<block_type> const &mem = Base::access_state().arena;

                            ASSERTF (num == 1, "can only allocate one object per call");
                            ASSERTF (mem.contains (free), "free list is corrupt");

                            block_type *block = free;
                            free = mem.items + free->index;

                            return reinterpret_cast <Type *> (block);
                        }

                        // deallocate
                        void deallocate (Type *ptr, size_t num)
                        {
                            block_type *&free = Base::access_state().head;
                            buffer<block_type> const &mem = Base::access_state().arena;

                            ASSERTF (num == 1, "can only allocate one object per call");
                            ASSERTF (mem.contains (ptr), "pointer is not from this heap");

                            block_type *block = reinterpret_cast <block_type *> (ptr);

                            block->index = free - mem.items;
                            free = block;
                        }

                    private:
                        void common_initialization ()
                        {
                            block_type *&free = Base::access_state().head;
                            buffer<block_type> const &mem = Base::access_state().arena;

                            ASSERTF (mem.items != nullptr, "memory not allocated");
                            ASSERTF (mem.size < (core::one << (sizeof(Index) * 8)), 
                                    "too many objects for size of free list index type");

                            free = mem.items;

                            Index index = 0;
                            block_type *block = mem.items;
                            block_type *end   = mem.items + mem.size;

                            for (; block < end; ++block)
                                block->index = ++index;
                        }

                        void common_finalization ()
                        {
                        }
                };
            }

            template <typename Base, typename Index>
            struct unity
            {
                template <typename T>
                struct state_type : get_state_type <Base, impl::block<Index,T>> 
                {
                    using base_type = get_state_type <Base, impl::block<Index,T>>;

                    impl::block<Index,T> *head;

                    state_type () :
                        head {nullptr} {}

                    state_type (state_type const &copy) :
                        base_type {copy}, head {copy.head} {}
                };

                template <typename S, typename T>
                using concrete_type = impl::unity <get_concrete_type <Base, S, impl::block<Index,T>>, S, T, Index>;
            
                using propagate_on_container_copy_assignment = std::true_type;
                using propagate_on_container_move_assignment = std::true_type;
                using propagate_on_container_swap = std::true_type;
            };
        }
    }
}


#endif
