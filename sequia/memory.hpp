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
    namespace memory
    {
        //=========================================================================

        template <typename T>
        struct buffer
        {
            size_t  size;   
            T       *mem;

            buffer () : 
                size (0), mem (0) {}

            buffer (size_t s, T *m) : 
                size (s), 
                mem (m) {}

            buffer (size_t s, void *m) : 
                size (s / sizeof(T)), 
                mem (static_cast<T *>(m)) {}
        };

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
                stateful_allocator_base (pointer p, size_type s) : buffer<T> (s, p) {}
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
                identity_allocator (pointer p, size_type s) : parent_type (p, s) {}

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
                unity_allocator (pointer p, size_type s);

                size_type max_size () const;
                pointer allocate (size_type num, const void* = 0);
                void deallocate (pointer ptr, size_type num);

            private:
                using parent_type::size;
                using parent_type::mem;

                pointer     pfree_;
                size_type   nfree_;
        };

        // Constructor

        template <typename T, typename IndexType>
        unity_allocator<T,IndexType>::unity_allocator (pointer p, size_type s) : 
            parent_type (p, s), pfree_ (p), nfree_ (s)
        {
            ASSERTF (size < (core::one << (sizeof(index_type) * 8)), 
                    "too many objects for size of free list index type");

            index_type i;
            pointer base = mem;

            for (i = 0; i < size; ++i)
                *(reinterpret_cast <index_type *> (base + i)) = i + 1;
        }

        // Copy Constructor

        template <typename T, typename IndexType>
        template <typename U> 
        unity_allocator<T,IndexType>::unity_allocator (unity_allocator<U, index_type> const &r) : 
            parent_type (r), pfree_ (r.pfree_), nfree_ (r.nfree_)
        {}

        // Capacity

        template <typename T, typename IndexType>
        auto unity_allocator<T,IndexType>::max_size () const -> size_type 
        { 
            return nfree_;
        }

        // Allocate

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

        // Deallocate

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

                static constexpr size_type freebit = core::one << ((sizeof(size_type) * 8) - 1);
                static constexpr size_type areabits = ~freebit;

                template <typename U> 
                struct rebind { typedef linear_allocator<U> other; };

                template <typename U> 
                linear_allocator (linear_allocator<U> const &r);
                linear_allocator (pointer pitems, size_type nitems, size_type *pallocs, size_type nallocs);

                size_type max_size () const;
                pointer allocate (size_type num, const void* = 0);
                void deallocate (pointer ptr, size_type num);

            private:
                using parent_type::size;
                using parent_type::mem;

                size_type       nfree_;
                descriptor_list list_;
        };

        // Constructor

        template <typename T>
        linear_allocator<T>::linear_allocator 
            (pointer pitems, size_type nitems, size_type *pallocs, size_type nallocs) : 
                parent_type (pitems, nitems), 
                nfree_ (nitems), 
                list_ (descriptor_allocator (pallocs, nallocs))
        {
            list_.push_back (nitems | freebit);
        }

        // Copy Constructor

        template <typename T>
        template <typename U> 
        linear_allocator<T>::linear_allocator (linear_allocator<U> const &r) : 
            parent_type (r), 
            nfree_ (r.nfree_), 
            list_ (r.list_)
        {}

        // Capacity

        template <typename T>
        auto linear_allocator<T>::max_size () const -> size_type 
        {
            return nfree_;
        }

        // Allocate

        template <typename T>
        auto linear_allocator<T>::allocate (size_type num, const void*) -> pointer 
        {
            ASSERTF (num < freebit, "allocation size impinges on free bit");

            nfree_ -= num;

            pointer ptr = 0;
            size_type free, area;
            typename descriptor_list::iterator descr = std::begin(list_); 
            typename descriptor_list::iterator end = std::end(list_); 

            for (pointer p = mem; descr != end; ++descr, p += area)
            {
                free = *descr & freebit;
                area = *descr & areabits; 

                if (free && area >= num)
                {
                    *descr = num;
                    area -= num;

                    if (area > 0) // fragment descriptor
                        list_.insert (descr+1, freebit | area);

                    ptr = p;
                    break;
                }
            }

            ASSERTF (descr != end, "unable to allocate pointer");
            ASSERTF ((ptr >= mem) && (ptr < mem + size), "free list is corrupt");

            return ptr;
        }

        // Deallocate

        template <typename T>
        auto linear_allocator<T>::deallocate (pointer ptr, size_type num) -> void
        {
            ASSERTF ((ptr >= mem) && (ptr < mem + size), "pointer is not from this heap");

            using std::remove;

            nfree_ += num;

            size_type free, area;
            typename descriptor_list::iterator begin = std::begin(list_); 
            typename descriptor_list::iterator descr = begin;
            typename descriptor_list::iterator end = std::end(list_); 
            typename descriptor_list::iterator next;

            for (pointer p = mem; descr != end; ++descr, p += area)
            {
                if (ptr == p)
                {
                    ASSERTF (!(*descr & freebit), "pointer was double-freed");

                    *descr |= freebit;

                    // merge neighboring descriptors

                    next = descr + 1;
                    free = *next & freebit;
                    area = *next & areabits; 

                    if (free && next != end)
                    {
                        *descr += area;
                        *next = 0; // clear for removal
                    }

                    next = descr--;
                    free = *descr & freebit;
                    area = *descr & areabits; 

                    if (free && next != begin)
                    {
                        *descr += area;
                        *next = 0; // clear for removal
                    }

                    break;
                }
            }

            ASSERTF (descr != end, "unable to deallocate pointer");

            // remove any invalid descriptors due to merge
            list_.erase (remove (descr, descr+3, 0), end(list_));
        }

        //-------------------------------------------------------------------------
        // TODO Heap Allocator


        //=========================================================================
        // Fixed Size Allocators

        //-------------------------------------------------------------------------

        template <typename T, size_t N>
        class fixed_identity_allocator : public identity_allocator<T>
        {
            public:
                typedef identity_allocator<T>       parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                static constexpr size_t mem_size = N * sizeof(value_type);

                template <typename U> 
                struct rebind { typedef fixed_identity_allocator<U, N> other; };

                template <typename U> 
                fixed_identity_allocator (fixed_identity_allocator<U, N> const &r) : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

                fixed_identity_allocator () : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

            private:
                uint8_t  mem_[mem_size]; // uninitialized memory
        };

        //-------------------------------------------------------------------------

        template <typename T, size_t N>
        class fixed_unity_allocator : public unity_allocator<T, typename core::min_word_size<N-1>::type>
        {
            public:
                typedef unity_allocator<T, typename core::min_word_size<N-1>::type> parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                static constexpr size_t mem_size = N * sizeof(value_type);

                template <typename U> 
                struct rebind { typedef fixed_unity_allocator<U, N> other; };

                template <typename U> 
                fixed_unity_allocator (fixed_unity_allocator<U, N> const &r) : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

                fixed_unity_allocator () : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

            private:
                uint8_t  mem_[mem_size]; // uninitialized memory
        };
    }
}

#endif
