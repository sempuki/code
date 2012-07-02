#ifndef _UNITY_ALLOCATOR_HPP_
#define _UNITY_ALLOCATOR_HPP_

#include <memory/fixed_allocator_base.hpp>

namespace sequia
{
    namespace memory
    {
        //-------------------------------------------------------------------------
        // Only allocates single element per call
        // Uses an embedded index-based linked free list

        template <typename T, typename IndexType, typename ParentAllocator = std::allocator<T>>
        using unity_parent_allocator = 
        typename ParentAllocator::rebind<block<T, IndexType>>::other;

        template <typename T, typename IndexType, typename ParentAllocator = std::allocator<T>>
        using unity_allocator_base = 
        fixed_allocator_base<block<T, IndexType>, unity_parent_allocator<T, IndexType, ParentAllocator>>

        template <typename T, typename IndexType, typename ParentAllocator = std::allocator<T>>
        class unity_allocator : 
            public unity_allocator_base<T, IndexType, ParentAllocator>
        {
            public:
                typedef ParentAllocator                                     parent_type;
                typedef unity_allocator_base<T, IndexType, ParentAllocator> base_type;

                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                typedef IndexType                                           index_type;
                typedef typename base_type::value_type                      block_type;
                typedef typename base_type::pointer                         block_pointer;
                typedef typename base_type::size_type                       block_size_type;

                // rebind type
                template <typename U> 
                struct rebind { typedef unity_allocator<U, IndexType, ParentAllocator> other; };

                // rebind constructor
                template <typename U> 
                unity_allocator (unity_allocator<U, IndexType, ParentAllocator> const &copy) :
                    base_type {copy} {}

                // default constructor
                unity_allocator () = delete;

                // copy constructor
                unity_allocator (unity_allocator const &copy) :
                    base_type {copy} 
                { 
                    init_memory_blocks (state().allocation); 
                }

                // stateful constructor
                explicit unity_allocator (State const &state) :
                    base_type {state} 
                { 
                    init_memory_blocks (state().allocation); 
                }

                // capacity
                size_type max_size () const
                { 
                    return nfree_;
                }

                // allocate
                pointer allocate (size_type num, const void*)
                {
                    ASSERTF (num == 1, "can only allocate one object per call");
                    ASSERTF (state().allocation.contains (pfree_), "free list is corrupt");

                    block_pointer block = pfree_;
                    block_pointer items = state().allocation.items;

                    pfree_ = items + pfree_->index;
                    nfree_--;

                    return reinterpret_cast <pointer> (block);
                }

                // deallocate
                void deallocate (pointer ptr, size_type num)
                {
                    ASSERTF (num == 1, "can only allocate one object per call");
                    ASSERTF (state().allocation.contains (ptr), "pointer is not from this heap");

                    block_pointer block = reinterpret_cast <block_pointer> (ptr);
                    block_pointer items = state().allocation.items;

                    block->index = pfree_ - items;
                    pfree_ = block;
                    nfree_++;
                }

            private:
                void init_memory_blocks (buffer<block_type> &buf);
                {
                    ASSERTF (buf.size < (core::one << (sizeof(index_type) * 8)), 
                            "too many objects for size of free list index type");

                    pfree_ = buf.items;
                    nfree_ = buf.size;

                    index_type index = 0;
                    pointer block = buf.items;
                    pointer end   = buf.items + buf.size;

                    for (; block < end; ++block)
                        block->index = index++;
                }

            private:
                block_pointer       pfree_;
                block_size_type     nfree_;
        };
    }
}

#endif
