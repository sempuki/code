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
            void deallocate (pointer p, size_type num);
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

    template <typename T>
    class fixed_allocator : public allocator_base <T>
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
            struct rebind { typedef fixed_allocator<U> other; };

            template <class U> 
            fixed_allocator (fixed_allocator<U> const &r) : buf_ (r.buf_) {}
            fixed_allocator (size_type size, void *mem) : buf_ (size, mem) {}

            size_type max_size () const { return buf_.size; }
            pointer allocate (size_type num, const void* = 0) { return buf_.mem; }
            void deallocate (pointer p, size_type num) {}

        private:
            buffer<T>   buf_;
    };

    template <class T1, class T2>
    bool operator== (fixed_allocator <T1> const &a, fixed_allocator <T2> const &b)
    {
        return a.buf_.mem == b.buf_.mem;
    }

    template <class T1, class T2>
    bool operator!= (fixed_allocator <T1> const &a, fixed_allocator <T2> const &b)
    {
        return a.buf_.mem != b.buf_.mem;
    }

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    class stack_allocator : public fixed_allocator <T>
    {
        public:
            typedef fixed_allocator <T>                     parent_type;
            typedef typename parent_type::value_type        value_type;
            typedef typename parent_type::pointer           pointer;
            typedef typename parent_type::const_pointer     const_pointer;
            typedef typename parent_type::reference         reference;
            typedef typename parent_type::const_reference   const_reference;
            typedef typename parent_type::size_type         size_type;
            typedef typename parent_type::difference_type   difference_type;

            template <class U> 
            struct rebind { typedef stack_allocator<U, N> other; };

            template <class U> 
            stack_allocator (stack_allocator<U, N> const &r) : fixed_allocator <T> (r) {}
            stack_allocator () : fixed_allocator <T> (N * sizeof(T), mem_) {}
        
        private:
            T   mem_[N];
    };

}

#endif
