#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

#include <cstddef>
#include <cstdio>

namespace sequia
{
    //=========================================================================

    constexpr uint64_t one = 1;

    constexpr size_t min_num_bytes (uint64_t value)
    {
       return 
           (value < (one << 8))? 1 : 
           (value < (one << 16) || 0 == (one << 16))? 2 : 
           (value < (one << 32) || 0 == (one << 32))? 4 : 8;
    }

    constexpr size_t min_num_bytes (int64_t value)
    {
        return 
            ((value > -(one << 7)) && (value < (one << 7)))? 1 :
            ((value > -(one << 15)) && (value < (one << 15)))? 2 :
            ((value > -(one << 31)) && (value < (one << 31)))? 4 : 8;
    }

    template <size_t N> struct min_word_type {};
    template <> struct min_word_type <1u> { typedef uint8_t result; };
    template <> struct min_word_type <2u> { typedef uint16_t result; };
    template <> struct min_word_type <4u> { typedef uint32_t result; };
    template <> struct min_word_type <8u> { typedef uint64_t result; };
    
    template <size_t N> 
    struct min_word_size 
    { 
        typedef typename min_word_type<min_num_bytes(N)>::result type;
    };

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

    //=========================================================================
    
    //-------------------------------------------------------------------------
    // Implements base allocator requirements

    template <class T>
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

    //template <typename AllocatorType, typename Interceptor>
    //class intercepting_allocator : public AllocatorType
    //{
    //    public:
    //        typedef AllocatorType                           parent_type;

    //        typedef typename parent_type::value_type        value_type;
    //        typedef typename parent_type::pointer           pointer;
    //        typedef typename parent_type::const_pointer     const_pointer;
    //        typedef typename parent_type::reference         reference;
    //        typedef typename parent_type::const_reference   const_reference;
    //        typedef typename parent_type::size_type         size_type;
    //        typedef typename parent_type::difference_type   difference_type;

    //        using Allocator::rebind::other;
    //        using Allocator::address;

    //        pointer address (reference value) const { return &value; }
    //        const_pointer address (const_reference value) const { return &value; }

    //        template<typename... Args>
    //        void construct (pointer p, Args&&... args) { new ((void *)p) T (std::forward<Args>(args)...); }
    //        void destroy (pointer p) { p->~T(); }

    //        size_type max_size () const;
    //        pointer allocate (size_type num, const void* hint = 0);
    //        void deallocate (pointer ptr, size_type num);
    //        template<typename... Args>
    //        void construct (pointer p, Args&&... args) 
    //        { 
    //            preconstruct(p, args...);

    //            postconstruct(p);
    //        }

    //        void destroy (pointer p) { p->~T(); }

    //        size_type max_size () const;
    //        pointer allocate (size_type num, const void* hint = 0);
    //        void deallocate (pointer ptr, size_type num);

    //    protected:
    //        Interceptor intercept_;
    //};

    //=========================================================================
    // Stateful Allocators
    //
    // NOTE: Stateful allocators are non-portable; however they should remain 
    // functional so long as one doesn't splice between node-based containers 
    // or swap whole containers.

    //-------------------------------------------------------------------------
    // base class for stateful allocators
    // TODO: C++11 constructor delegation for subclasses

    template <class T>
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

            template <typename U> struct rebind { typedef stateful_allocator_base<U> other; };

            template <class U> 
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

            template <class U> 
            struct rebind { typedef identity_allocator<U> other; };

            template <class U> 
            identity_allocator (identity_allocator<U> const &r) : parent_type (r) {}
            identity_allocator (size_type s, pointer p) : parent_type (s, p) {}

            size_type max_size () const 
            { 
                return size;
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                assert (num <= max_size());

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
                    "sizeof(T) too small for free list index.");

            template <typename U> 
            struct rebind { typedef unity_allocator<U, index_type> other; };

            template <typename U> 
            unity_allocator (unity_allocator<U, index_type> const &r) : 
                parent_type (r) {}
    
            unity_allocator (size_type s, pointer p) : 
                parent_type (s, p), nfree_ (s), pfree_ (p)
            {
                assert (size < (one << (sizeof(index_type) * 8)));

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
                assert (num == 1);
                assert ((pfree_ >= mem) && (pfree_ < mem + size));

                index_type i = *(reinterpret_cast <index_type *> (pfree_));
                pointer ptr = pfree_;
                pfree_ = mem + i;
                nfree_--;

                return ptr;
            }

            void deallocate (pointer ptr, size_type num) 
            {
                assert (num == 1);
                assert ((ptr >= mem) && (ptr < mem + size));

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

            template <typename U> 
            struct rebind { typedef linear_allocator<U> other; };

            template <typename U> 
            linear_allocator (linear_allocator<U> const &r) : 
                parent_type (r) {}
    
            linear_allocator (size_type s, pointer p) : 
                parent_type (s, p)
            {
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
