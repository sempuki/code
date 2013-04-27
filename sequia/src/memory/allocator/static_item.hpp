#ifndef _STATIC_ITEM_ALLOCATOR_HPP_
#define _STATIC_ITEM_ALLOCATOR_HPP_

namespace sequia { namespace memory { namespace allocator {

    //=====================================================================
    // Allocates the one item from a static buffer each call

    namespace impl
    {
        // Embeds an index-based linked list within the buffer
        template <typename Index, typename Type>
        union block
        {
            using index_type = Index;
            using value_type = Type;

            index_type  index;
            value_type  value;

            block() : index {0} {}
        };

        template <typename T, size_t N>
        using block_type = block <typename core::min_word_size<N-1>::type, T>;
        
        template <typename T>
        using block_32 = block <uint32_t, T>;
    }

    template <typename Type, size_t N>
    class static_item : public std::allocator <impl::block_type<Type,N>>
    {
        public:
            template <class U> struct rebind { using other = static_item<U,N>; };

        public:
            static_item ()
            {
                ASSERTF (mem_, "memory not allocated");
                ASSERTF (N < (core::one << (core::min_num_bytes(N)*8)),
                        "too many objects for size of free list index type");
                
                auto index = 0;
                for (auto &block : mem_)
                    block.index = ++index;

                head_ = begin (mem_);
            }

            static_item (static_item const &copy) :
                static_item {} 
            {
                WATCHF (false, "allocator copy constructor called");
            }

            template <class U>
            static_item (static_item<U,N> const &copy) :
                static_item {} 
            {
                WATCHF (false, "allocator rebind copy constructor called");
            }

        public:
            size_t max_size () const 
            { 
                return N;
            }

            Type *allocate (size_t num, const void* = 0) 
            { 
                ASSERTF (num == 1, "can only allocate one object per call");
                ASSERTF (contains (mem_, head_), "free list is corrupt");

                auto block = head_;
                head_ = begin (mem_) + head_->index;

                return reinterpret_cast <Type *> (block);
            }

            void deallocate (Type *ptr, size_t num) 
            {
                auto block = reinterpret_cast <impl::block_type<Type,N> *> (ptr);

                ASSERTF (num == 1, "can only allocate one object per call");
                ASSERTF (mem_, "not previously allocated");
                ASSERTF (contains (mem_, block), "pointer is not from this heap");

                block->index = head_ - begin (mem_);
                head_ = block;
            }

        private:
            impl::block_type<Type,N> *head_;
            memory::static_buffer<impl::block_type<Type,N>,N> mem_;
    };
        
} } }

#endif
