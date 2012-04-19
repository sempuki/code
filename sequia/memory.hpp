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

            template <class U> struct rebind { typedef allocator_base<U> other; };

            pointer address (reference value) const { return &value; }
            const_pointer address (const_reference value) const { return &value; }

            template<typename... Args>
            void construct (pointer p, Args&&... args) { new ((void *)p) T (std::forward<Args>(args)...); }
            void destroy (pointer p) { p->~T(); }

            size_type max_size () const;
            pointer allocate (size_type num, const void* hint = 0);
            void deallocate (pointer ptr, size_type num);
    };

    template <class T1, class T2>
    bool operator== (const allocator_base<T1>&, const allocator_base<T2>&)
    {
        return true;
    }

    template <class T1, class T2>
    bool operator!= (const allocator_base<T1>&, const allocator_base<T2>&)
    {
        return false;
    }

    //-------------------------------------------------------------------------
    // Used to pass a specifed allocator during rebind

    template <typename T, typename AllocatorType>
    class rebind_allocator : public allocator_base <T>
    {
        public:
            typedef allocator_base <T>                      parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <class U> 
            struct rebind { typedef AllocatorType other; };
    };

    //=========================================================================
    // Stateful Allocators
    //
    // NOTE: Stateful allocators are non-portable; however they should remain 
    // functional so long as one doesn't splice between node-based containers 
    // or swap whole containers.

    //-------------------------------------------------------------------------
    // Only allocates the same buffer for each call

    template <typename T>
    class identity_allocator : public allocator_base <T>
    {
        public:
            typedef allocator_base <T>                      parent_type;

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
            identity_allocator (identity_allocator<U> const &r) : buf_ (r.buf_) {}
            identity_allocator (size_type size, pointer mem) : buf_ (size, mem) {}

            size_type max_size () const 
            { 
                return buf_.size;
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                assert (num <= max_size());

                return buf_.mem; 
            }

            void deallocate (pointer ptr, size_type num) 
            {}

        private:
            buffer<value_type>  buf_;
    };

    template <class T1, class T2>
    bool operator== (identity_allocator <T1> const &a, identity_allocator <T2> const &b)
    {
        return a.buf_.mem == b.buf_.mem;
    }

    template <class T1, class T2>
    bool operator!= (identity_allocator <T1> const &a, identity_allocator <T2> const &b)
    {
        return a.buf_.mem != b.buf_.mem;
    }


    //-------------------------------------------------------------------------
    // Only allocates single element per call

    template <typename T, typename IndexType>
    class unity_allocator : public allocator_base <T>
    {
        public:
            typedef allocator_base <T>                      parent_type;

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

            template <class U> 
            struct rebind { typedef unity_allocator<U, index_type> other; };

            template <class U> 
            unity_allocator (unity_allocator<U, index_type> const &r) : 
                buf_ (r.buf_) {}
    
            unity_allocator (size_type size, pointer mem) : 
                buf_ (size, mem), nfree_ (size), pfree_ (mem)
            {
                assert (size < (one << (sizeof(index_type) * 8)));

                index_type i;
                pointer base = buf_.mem;

                for (i = 0; i < size - 1; ++i)
                {
                    printf("0x%x <- %d\n", base + i, i + 1);
                    *(reinterpret_cast <index_type *> (base + i)) = i + 1;
                }

                printf("0x%x <- %d\n", base + i, 0);
                *(reinterpret_cast <index_type *> (base + i)) = 0;
            }

            size_type max_size () const 
            { 
                return nfree_;
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                assert (num == 1);

                index_type i = *(reinterpret_cast <index_type *> (pfree_));
                printf("0x%x -> %d\n", pfree_, i);
                assert (i < buf_.size);

                pointer ptr = pfree_;
                pfree_ = buf_.mem + i;
                nfree_--;

                return ptr;
            }

            void deallocate (pointer ptr, size_type num) 
            {
                assert (num == 1);

                index_type i = pfree_ - buf_.mem;
                assert (i < buf_.size);

                *(reinterpret_cast <index_type *> (ptr)) = i;
                pfree_ = ptr;
                nfree_++;
            }

        private:
            buffer<value_type>  buf_;
            size_type           nfree_;
            pointer             pfree_;
    };

    template <typename T1, typename I1, typename T2, typename I2>
    bool operator== (unity_allocator <T1, I1> const &a, unity_allocator <T2, I2> const &b)
    {
        return a.buf_.mem == b.buf_.mem;
    }

    template <typename T1, typename I1, typename T2, typename I2>
    bool operator!= (unity_allocator <T1, I2> const &a, unity_allocator <T2, I2> const &b)
    {
        return a.buf_.mem != b.buf_.mem;
    }

    //-------------------------------------------------------------------------

    template <size_t N, typename T>
    class fixed_identity_allocator : public identity_allocator <T>
    {
        public:
            typedef identity_allocator <T>                  parent_type;

            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <class U> 
            struct rebind { typedef fixed_identity_allocator<N, U> other; };

            template <class U> 
            fixed_identity_allocator (fixed_identity_allocator<N, U> const &r) : parent_type (N, mem_) {}
            fixed_identity_allocator () : parent_type (N, mem_) {}
        
        private:
            value_type  mem_[N];
    };


    template <size_t N, typename T>
    class fixed_unity_allocator : public unity_allocator <T, typename min_word_size<N>::type>
    {
        public:
            typedef unity_allocator<T, typename min_word_size<N>::type> parent_type;

            typedef typename parent_type::index_type        index_type;
            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <class U> 
            struct rebind { typedef fixed_unity_allocator<N, U> other; };

            template <class U> 
            fixed_unity_allocator (fixed_unity_allocator<N, U> const &r) : parent_type (N, mem_) {}
            fixed_unity_allocator () : parent_type (N, mem_) {}
        
        private:
            value_type  mem_[N];
    };

}

#endif
