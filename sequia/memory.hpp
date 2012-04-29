#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

#include <vector>
#include "core.hpp"

namespace sequia
{
    //=========================================================================
    
    //-------------------------------------------------------------------------
    // Implements base allocator requirements

    template <typename T>
    class allocator_base 
    {
        public:
            typedef T           value_type;
            typedef T*          pointer;
            typedef const T*    const_pointer;
            typedef T&          reference;
            typedef const T&    const_reference;
            typedef size_t      size_type;
            typedef ptrdiff_t   difference_type;

            template <typename U> struct rebind { typedef allocator_base<U> other; };

            pointer address (reference value) const { return &value; }
            const_pointer address (const_reference value) const { return &value; }

            template<typename... Args>
            void construct (pointer p, Args&&... args) { new ((void *)p) T (std::forward<Args>(args)...); }
            void destroy (pointer p) { p->~T(); }

            size_type max_size () const;
            pointer allocate (size_type num, const void* hint = 0);
            void deallocate (pointer ptr, size_type num);
    };

    template <typename T1, typename T2>
    bool operator== (allocator_base<T1> const &a, allocator_base<T2> const &b)
    {
        return true;
    }

    template <typename T1, typename T2>
    bool operator!= (allocator_base<T1> const &a, allocator_base<T2> const &b)
    {
        return false;
    }

    //-------------------------------------------------------------------------
    // Used to pass a specifed allocator during rebind

    template <typename AllocatorType>
    class rebind_allocator
    {
        public:
            typedef typename AllocatorType::value_type      value_type;
            typedef typename AllocatorType::pointer         pointer;
            typedef typename AllocatorType::const_pointer   const_pointer;
            typedef typename AllocatorType::reference       reference;
            typedef typename AllocatorType::const_reference const_reference;
            typedef typename AllocatorType::size_type       size_type;
            typedef typename AllocatorType::difference_type difference_type;

            template <typename U> 
            struct rebind { typedef AllocatorType other; };
    };

    //-------------------------------------------------------------------------
    // Used to intercept allocator calls

    template <typename AllocatorType, typename Interceptor>
    class intercepting_allocator : public AllocatorType, private Interceptor
    {
        public:
            typedef AllocatorType                           parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            using AllocatorType::rebind::other;
            using AllocatorType::address;
            using AllocatorType::max_size;
            using AllocatorType::construct;
            using AllocatorType::destroy;

            pointer allocate (size_type num, const void* hint = 0)
            {
                pointer ptr = AllocatorType::allocate(num, hint);
                return Interceptor::on_allocate(ptr, num, hint);
            }

            void deallocate (pointer ptr, size_type num)
            {
                AllocatorType::deallocate(ptr, num);
                Interceptor::on_deallocate(ptr, num);
            }
    };

    //=========================================================================
    // Stateful Allocators
    //
    // NOTE: Stateful allocators are non-portable; however they should remain 
    // functional so long as one doesn't splice between node-based containers 
    // or swap whole containers.

    //-------------------------------------------------------------------------
    // base class for stateful allocators
    // TODO: C++11 constructor delegation for subclasses

    template <typename T>
    class stateful_allocator_base : public allocator_base<T>, protected buffer<T>
    {
        public:
            typedef allocator_base<T>                       parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <typename U> 
            struct rebind { typedef stateful_allocator_base<U> other; };

            template <typename U> 
            stateful_allocator_base (stateful_allocator_base<U> const &r) : buffer<T> (r) {}
            stateful_allocator_base (size_type s, pointer p) : buffer<T> (s, p) {}
    };

    template <typename T1, typename T2>
    bool operator== (stateful_allocator_base<T1> const &a, stateful_allocator_base<T2> const &b)
    {
        return a.mem == b.mem;
    }

    template <typename T1, typename T2>
    bool operator!= (stateful_allocator_base<T1> const &a, stateful_allocator_base<T2> const &b)
    {
        return a.mem != b.mem;
    }

    //-------------------------------------------------------------------------
    // Only allocates the same buffer for each call

    template <typename T>
    class identity_allocator : public stateful_allocator_base<T>
    {
        public:
            typedef stateful_allocator_base<T>              parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <typename U> 
            struct rebind { typedef identity_allocator<U> other; };

            template <typename U> 
            identity_allocator (identity_allocator<U> const &r) : parent_type (r) {}
            identity_allocator (size_type s, pointer p) : parent_type (s, p) {}

            size_type max_size () const 
            { 
                return size;
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                return mem; 
            }

            void deallocate (pointer ptr, size_type num) 
            {}

        private:
            using parent_type::size;
            using parent_type::mem;
    };


    //-------------------------------------------------------------------------
    // Only allocates single element per call
    // Uses an embedded index-based linked free list

    template <typename T, typename IndexType>
    class unity_allocator : public stateful_allocator_base<T>
    {
        public:
            typedef stateful_allocator_base<T>              parent_type;

            typedef IndexType                               index_type;
            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            static_assert (sizeof(value_type) >= sizeof(index_type), 
                    "sizeof(T) too small for free list index");

            template <typename U> 
            struct rebind { typedef unity_allocator<U, index_type> other; };

            template <typename U> 
            unity_allocator (unity_allocator<U, index_type> const &r) : 
                parent_type (r) {}
    
            unity_allocator (size_type s, pointer p) : 
                parent_type (s, p), nfree_ (s), pfree_ (p)
            {
                ASSERTF (size < (one << (sizeof(index_type) * 8)), 
                        "too many objects for size of free list index type");

                index_type i;
                pointer base = mem;

                for (i = 0; i < size; ++i)
                    *(reinterpret_cast <index_type *> (base + i)) = i + 1;
            }

            size_type max_size () const 
            { 
                return nfree_;
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                ASSERTF (num == 1, "can only allocate one object per call");
                ASSERTF ((pfree_ >= mem) && (pfree_ < mem + size), "free list is corrupt");

                index_type i = *(reinterpret_cast <index_type *> (pfree_));
                pointer ptr = pfree_;
                pfree_ = mem + i;
                nfree_--;

                return ptr;
            }

            void deallocate (pointer ptr, size_type num) 
            {
                ASSERTF (num == 1, "can only allocate one object per call");
                ASSERTF ((ptr >= mem) && (ptr < mem + size), "pointer is not from this heap");

                index_type i = pfree_ - mem;
                *(reinterpret_cast <index_type *> (ptr)) = i;
                pfree_ = ptr;
                nfree_++;
            }

        private:
            using parent_type::size;
            using parent_type::mem;

            size_type           nfree_;
            pointer             pfree_;
    };

    //-------------------------------------------------------------------------
    // Does linear search of allocation descriptor vector on de/allocation
    // Intended for a small number of variable-size allocations
    // Note: least significant bit encodes whether the block is free or used
    // which results in a minimum two-item allocation size

    template <typename T>
    class linear_allocator : public stateful_allocator_base<T>
    {
        public:
            typedef stateful_allocator_base<T>              parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            typedef identity_allocator<size_type> DescriptorAllocator;
            typedef std::vector<size_type, DescriptorAllocator> DescriptorList;

            static constexpr size_type flagused = 1;
            static constexpr size_type flagfree = ~flagused;

            template <typename U> 
            struct rebind { typedef linear_allocator<U> other; };

            template <typename U> 
            linear_allocator (linear_allocator<U> const &r) : 
                parent_type (r) {}
    
            linear_allocator (size_type nitems, pointer pitems, size_type nallocs, pointer pallocs) : 
                parent_type (nitems, pitems), 
                descr_ (DescriptorAllocator (pallocs, nallocs * sizeof(size_type)))
            {
                //descr_.push_back(nitems & freeflag);
            }

            size_type max_size () const 
            {
            }

            pointer allocate (size_type num, const void* = 0) 
            {
            }

            void deallocate (pointer ptr, size_type num) 
            {
            }

        private:
            using parent_type::size;
            using parent_type::mem;

            DescriptorList descr_;
    };

    //-------------------------------------------------------------------------

    template <size_t N, typename T>
    class fixed_identity_allocator : public identity_allocator<T>
    {
        public:
            typedef identity_allocator<T>                   parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            static constexpr size_t mem_size = N * sizeof(value_type);

            template <typename U> 
            struct rebind { typedef fixed_identity_allocator<N, U> other; };

            template <typename U> 
            fixed_identity_allocator (fixed_identity_allocator<N, U> const &r) : 
                parent_type (N, reinterpret_cast <pointer> (mem_)) {}

            fixed_identity_allocator () : 
                parent_type (N, reinterpret_cast <pointer> (mem_)) {}
        
        private:
            uint8_t  mem_[mem_size]; // uninitialized memory
    };


    template <size_t N, typename T>
    class fixed_unity_allocator : public unity_allocator<T, typename min_word_size<N-1>::type>
    {
        public:
            typedef unity_allocator<T, typename min_word_size<N-1>::type> parent_type;

            typedef typename parent_type::index_type        index_type;
            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            static constexpr size_t mem_size = N * sizeof(value_type);

            template <typename U> 
            struct rebind { typedef fixed_unity_allocator<N, U> other; };

            template <typename U> 
            fixed_unity_allocator (fixed_unity_allocator<N, U> const &r) : 
                parent_type (N, reinterpret_cast <pointer> (mem_)) {}

            fixed_unity_allocator () : 
                parent_type (N, reinterpret_cast <pointer> (mem_)) {}
        
        private:
            uint8_t  mem_[mem_size]; // uninitialized memory
    };

}

#endif
