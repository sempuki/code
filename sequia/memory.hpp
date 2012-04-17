#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

#include <cstddef>

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

    //=========================================================================
    
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
    // Only allocates single elements per call

    template <typename T>
    class unity_allocator : public allocator_base <T>
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
            struct rebind { typedef unity_allocator<U> other; };

            template <class U> 
            unity_allocator (unity_allocator<U> const &r) : 
                buf_ (r.buf_) {}
    
            unity_allocator (size_type size, pointer mem) : 
                buf_ (size, mem), nfree_ (size), pfree_ (mem)
            {
                // build implicit linked free list
                pointer ptr = mem; 
                pointer end = mem + (size - 1);
                for (; ptr < end; ++ptr)
                    *(reinterpret_cast <pointer *> (ptr)) = ptr+1;
                *(reinterpret_cast <pointer *> (ptr)) = 0;
            }

            size_type max_size () const 
            { 
                return nfree_;
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                assert (num == 1);

                pointer ptr = pfree_;
                pfree_ = *(reinterpret_cast <pointer *> (pfree_));
                nfree_--;

                return ptr;
            }

            void deallocate (pointer ptr, size_type num) 
            {
                assert (num == 1);

                *(reinterpret_cast <pointer *> (ptr)) = pfree_;
                pfree_ = ptr;
                nfree_++;
            }

        private:
            buffer<value_type>  buf_;
            size_type           nfree_;
            pointer             pfree_;
    };

    template <class T1, class T2>
    bool operator== (unity_allocator <T1> const &a, unity_allocator <T2> const &b)
    {
        return a.buf_.mem == b.buf_.mem;
    }

    template <class T1, class T2>
    bool operator!= (unity_allocator <T1> const &a, unity_allocator <T2> const &b)
    {
        return a.buf_.mem != b.buf_.mem;
    }

    //-------------------------------------------------------------------------

    template <size_t N, typename T>
    class stack_identity_allocator : public identity_allocator <T>
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
            struct rebind { typedef stack_identity_allocator<N, U> other; };

            template <class U> 
            stack_identity_allocator (stack_identity_allocator<N, U> const &r) : parent_type (N, mem_) {}
            stack_identity_allocator () : parent_type (N, mem_) {}
        
        private:
            value_type  mem_[N];
    };


    template <size_t N, typename T>
    class stack_unity_allocator : public unity_allocator <T>
    {
        public:
            typedef unity_allocator <T>                     parent_type;
            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <class U> 
            struct rebind { typedef stack_unity_allocator<N, U> other; };

            template <class U> 
            stack_unity_allocator (stack_unity_allocator<N, U> const &r) : parent_type (N, mem_) {}
            stack_unity_allocator () : parent_type (N, mem_) {}
        
        private:
            value_type  mem_[N];
    };

}

#endif
