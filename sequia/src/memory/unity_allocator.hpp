#ifndef _UNITY_ALLOCATOR_HPP_
#define _UNITY_ALLOCATOR_HPP_

#include <memory/stateful_allocator_base.hpp>

namespace sequia
{
    namespace memory
    {
        //-------------------------------------------------------------------------
        // Only allocates single element per call
        // Uses an embedded index-based linked free list

        template <typename T, typename IndexType>
        class unity_allocator : public stateful_allocator_base<T>
        {
            public:
                typedef unity_allocator<T, IndexType> this_type;
                typedef stateful_allocator_base<T>  parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                typedef IndexType                   index_type;

                static_assert (sizeof(value_type) >= sizeof(index_type), 
                        "sizeof(T) too small for free list index");

                template <typename U> 
                struct rebind { typedef unity_allocator<U, IndexType> other; };

                template <typename U> 
                unity_allocator (unity_allocator<U, IndexType> const &r);
                unity_allocator (pointer p, size_type s);

                size_type max_size () const;
                pointer allocate (size_type num, const void* = 0);
                void deallocate (pointer ptr, size_type num);

                static size_type calc_size (pointer p, size_type s);

            private:
                pointer     pfree_;
                size_type   nfree_;
        };

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
        // Constructor

        template <typename T, typename IndexType>
        unity_allocator<T, IndexType>::
        unity_allocator (pointer p, size_type s) : 
            parent_type {p, s}, 
            pfree_ {p}, 
            nfree_ {s}
        {
            ASSERTF (buffer<T>::size < (core::one << (sizeof(index_type) * 8)), 
                    "too many objects for size of free list index type");

            index_type i;
            pointer base = buffer<T>::mem;

            for (i = 0; i < buffer<T>::size; ++i)
                *(reinterpret_cast <index_type *> (base + i)) = i + 1;
        }

        // Rebind Copy Constructor

        template <typename T, typename IndexType>
        template <typename U> 
        unity_allocator<T, IndexType>::
        unity_allocator (unity_allocator<U, index_type> const &r) : 
            parent_type {r}, 
            pfree_ {r.pfree_}, 
            nfree_ {r.nfree_}
        {}

        // Capacity

        template <typename T, typename IndexType>
        auto unity_allocator<T, IndexType>::
        max_size () const -> size_type 
        { 
            return nfree_;
        }

        // Allocate

        template <typename T, typename IndexType>
        auto unity_allocator<T, IndexType>::
        allocate (size_type num, const void*) -> pointer 
        { 
            ASSERTF (num == 1, "can only allocate one object per call");
            ASSERTF ((pfree_ >= buffer<T>::mem) && 
                    (pfree_ < buffer<T>::mem + buffer<T>::size), 
                    "free list is corrupt");

            index_type i = *(reinterpret_cast <index_type *> (pfree_));
            pointer ptr = pfree_;
            pfree_ = buffer<T>::mem + i;
            nfree_--;

            return ptr;
        }

        // Deallocate

        template <typename T, typename IndexType>
        auto unity_allocator<T, IndexType>::
        deallocate (pointer ptr, size_type num) -> void
        {
            ASSERTF (num == 1, "can only allocate one object per call");
            ASSERTF ((ptr >= buffer<T>::mem) && 
                    (ptr < buffer<T>::mem + buffer<T>::size), 
                    "pointer is not from this heap");

            index_type i = pfree_ - buffer<T>::mem;
            *(reinterpret_cast <index_type *> (ptr)) = i;
            pfree_ = ptr;
            nfree_++;
        }
                
        template <typename T, typename IndexType>
        auto unity_allocator<T, IndexType>::
        calc_size (pointer p, size_type n) -> size_type 
        { 
            return sizeof(this_type) + sizeof(T) * n;
        }

    }
}

#endif
