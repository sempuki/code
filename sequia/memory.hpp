#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

#include "core.hpp"

#define DECLARE_INHERITED_ALLOCATOR_TYPES_(AllocatorType) \
    typedef typename AllocatorType::value_type      value_type; \
    typedef typename AllocatorType::pointer         pointer; \
    typedef typename AllocatorType::const_pointer   const_pointer; \
    typedef typename AllocatorType::reference       reference; \
    typedef typename AllocatorType::const_reference const_reference; \
    typedef typename AllocatorType::size_type       size_type; \
    typedef typename AllocatorType::difference_type difference_type;

namespace sequia
{
    //=========================================================================
    
    template <typename T>
    struct buffer
    {
        size_t  size;   
        T       *mem;

        buffer () : size (0), mem (0) {}
        buffer (size_t s, T *m) : size (s), mem (m) {}
        buffer (size_t s, void *m) : size (s / sizeof(T)), mem (static_cast<T *>(m)) {}
    };

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

            template <typename U> 
            struct rebind { typedef allocator_base<U> other; };

            pointer address (reference value) const { return &value; }
            const_pointer address (const_reference value) const { return &value; }

            template<typename... Args>
            void construct (pointer p, Args&&... args) 
            { 
                new ((void *)p) T (std::forward<Args>(args)...); 
            }

            void destroy (pointer p) 
            { 
                p->~T(); 
            }

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
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (AllocatorType);

            template <typename U> 
            struct rebind { typedef AllocatorType other; };
    };

    //-------------------------------------------------------------------------
    // Used to intercept allocator calls

    template <typename AllocatorType, typename Interceptor>
    class intercepting_allocator : public AllocatorType, private Interceptor
    {
        public:
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (AllocatorType);

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
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (allocator_base<T>);

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
            typedef stateful_allocator_base<T>  parent_type;
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

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
            typedef stateful_allocator_base<T>  parent_type;
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

            typedef IndexType                   index_type;

            static_assert (sizeof(value_type) >= sizeof(index_type), 
                    "sizeof(T) too small for free list index");

            template <typename U> 
            struct rebind { typedef unity_allocator<U, index_type> other; };

            template <typename U> 
            unity_allocator (unity_allocator<U, index_type> const &r);
            unity_allocator (size_type s, pointer p);

            size_type max_size () const;
            pointer allocate (size_type num, const void* = 0);
            void deallocate (pointer ptr, size_type num);

        private:
            using parent_type::size;
            using parent_type::mem;

            size_type   nfree_;
            pointer     pfree_;
    };

    template <typename T, typename IndexType>
        unity_allocator<T,IndexType>::unity_allocator (size_type s, pointer p) 
        : parent_type (s, p), nfree_ (s), pfree_ (p)
        {
            ASSERTF (size < (one << (sizeof(index_type) * 8)), 
                    "too many objects for size of free list index type");

            index_type i;
            pointer base = mem;

            for (i = 0; i < size; ++i)
                *(reinterpret_cast <index_type *> (base + i)) = i + 1;
        }

            
    template <typename T, typename IndexType>
        template <typename U> 
        unity_allocator<T,IndexType>::unity_allocator (unity_allocator<U, index_type> const &r) 
        : parent_type (r) 
        {}


    template <typename T, typename IndexType>
        auto unity_allocator<T,IndexType>::max_size () const -> size_type 
        { 
            return nfree_;
        }


    template <typename T, typename IndexType>
        auto unity_allocator<T,IndexType>::allocate (size_type num, const void*) -> pointer 
        { 
            ASSERTF (num == 1, "can only allocate one object per call");
            ASSERTF ((pfree_ >= mem) && (pfree_ < mem + size), "free list is corrupt");

            index_type i = *(reinterpret_cast <index_type *> (pfree_));
            pointer ptr = pfree_;
            pfree_ = mem + i;
            nfree_--;

            return ptr;
        }


    template <typename T, typename IndexType>
        auto unity_allocator<T,IndexType>::deallocate (pointer ptr, size_type num) -> void
        {
            ASSERTF (num == 1, "can only allocate one object per call");
            ASSERTF ((ptr >= mem) && (ptr < mem + size), "pointer is not from this heap");

            index_type i = pfree_ - mem;
            *(reinterpret_cast <index_type *> (ptr)) = i;
            pfree_ = ptr;
            nfree_++;
        }


    //-------------------------------------------------------------------------
    // Does linear first-fit search of allocation descriptor vector
    // Intended for a small number of variable-size allocations

    template <typename T>
    class linear_allocator : public stateful_allocator_base<T>
    {
        public:
            typedef stateful_allocator_base<T>  parent_type;
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

            typedef identity_allocator<size_type> descriptor_allocator;
            typedef std::vector<size_type, descriptor_allocator> descriptor_list;

            static constexpr size_type freebit = one << ((sizeof(size_type) * 8) - 1);
            static constexpr size_type sizebits = ~freebit;

            template <typename U> 
            struct rebind { typedef linear_allocator<U> other; };

            template <typename U> 
            linear_allocator (linear_allocator<U> const &r);
            linear_allocator (size_type nitems, pointer pitems, size_type nallocs, size_type *pallocs);

            size_type max_size () const;
            pointer allocate (size_type num, const void* = 0);
            void deallocate (pointer ptr, size_type num);

        private:
            using parent_type::size;
            using parent_type::mem;

            descriptor_list list_;
            size_type       nfree_;
    };

    template <typename T>
        template <typename U> 
        linear_allocator<T>::linear_allocator (linear_allocator<U> const &r) 
        : parent_type (r) 
        {}
    

    template <typename T>
        linear_allocator<T>::linear_allocator (size_type nitems, pointer pitems, size_type nallocs, size_type *pallocs) 
        : parent_type (nitems, pitems), list_ (descriptor_allocator (nallocs, pallocs)), nfree_ (nitems)
        {
            list_.push_back (nitems | freebit);
        }


    template <typename T>
        linear_allocator<T>::max_size () const -> size_type 
        {
            return nfree_;
        }

    
    template <typename T>
        linear_allocator<T>::allocate (size_type num, const void*) -> pointer 
        {
            ASSERTF (num < freebit, "allocation size impinges on free bit");

            nfree_ -= num;

            pointer ptr;
            size_type free, size;
            typename descriptor_list::iterator descr = begin(list_); 
            typename descriptor_list::iterator end = end(list_); 

            for (pointer p = mem; descr != end; ++descr, p += size)
            {
                free = *descr & freebit;
                size = *descr & sizebits; 

                if (free && size >= num)
                {
                    *descr = num;
                    size -= num;

                    if (size > 0) // fragment descriptor
                        list_.insert (++descr, freebit | size);

                    ptr = p;
                    break;
                }
            }

            ASSERTF ((ptr >= mem) && (ptr < mem + size), "free list is corrupt");

            return ptr;
        }


    template <typename T>
        linear_allocator<T>::deallocate (pointer ptr, size_type num) -> void
        {
            ASSERTF ((ptr >= mem) && (ptr < mem + size), "pointer is not from this heap");

            using std::remove;

            nfree_ += num;

            size_type free, size;
            typename descriptor_list::iterator begin = begin(list_); 
            typename descriptor_list::iterator descr = begin;
            typename descriptor_list::iterator end = end(list_); 
            typename descriptor_list::iterator next;

            for (pointer p = mem; descr != end; ++descr, p += size)
            {
                if (ptr == p)
                {
                    ASSERTF (!(*descr & freebit), "pointer was double-freed");

                    *descr |= freebit;

                    // merge neighboring descriptors

                    next = descr + 1;
                    free = *next & freebit;
                    size = *next & sizebits; 

                    if (free && next != end)
                    {
                        *descr += size;
                        *next = 0; // clear for removal
                    }

                    next = descr--;
                    free = *descr & freebit;
                    size = *descr & sizebits; 

                    if (free && next != begin)
                    {
                        *descr += size;
                        *next = 0; // clear for removal
                    }

                    break;
                }
            }

            ASSERTF (descr != end, "unable to free pointer");

            // remove any invalid descriptors due to merge
            list_.erase (remove (descr, descr+3, 0), end(list_));
        }

    //-------------------------------------------------------------------------
    // TODO Heap Allocator
    
    //=========================================================================
    // Fixed Size Allocators
    //

    template <size_t N, typename T>
    class fixed_identity_allocator : public identity_allocator<T>
    {
        public:
            typedef identity_allocator<T>       parent_type;
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

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

    //-------------------------------------------------------------------------

    template <size_t N, typename T>
    class fixed_unity_allocator : public unity_allocator<T, typename min_word_size<N-1>::type>
    {
        public:
            typedef unity_allocator<T, typename min_word_size<N-1>::type> parent_type;
            DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

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
