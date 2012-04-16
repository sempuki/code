#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

namespace sequia
{
    template <typename T>
    struct buffer
    {
        size_t  size;
        T       *data;
    
        buffer (size_t s, T *b) : size (s), data (b) {}
    };

    template <class T>
    class allocator_base 
    {
        public:
            typedef T                value_type;
            typedef T*               pointer;
            typedef const T*         const_pointer;
            typedef T&               reference;
            typedef const T&         const_reference;
            typedef std::size_t      size_type;
            typedef std::ptrdiff_t   difference_type;

            pointer address (reference value) const { return &value; }
            const_pointer address (const_reference value) const { return &value; }

            template<typename... Args>
            void construct (pointer p, Args&&... args) { new ((void *)p) T (std::forward<Args>(args)...); }
            void destroy (pointer p) { p->~T(); }

            template <class U> struct rebind { typedef allocator_base<U> other; };
            template <class U> allocator_base (allocator_base<U> const &) {}

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

    template <typename T>
    class fixed_allocator : public allocator_base <T>
    {
        public:
            template <class U> 
            struct rebind { typedef allocator_base<U> other; };

            template <class U> 
            fixed_allocator (fixed_allocator<U> const &r) : 
                mem_ (r.mem_), size_ (r.size_) 
            {}

            fixed_allocator (void *m, size_t s) : 
                mem_ (m), size_ (s) {}

            size_type max_size () const 
            { 
                return size_; 
            }

            pointer allocate (size_type num, const void* = 0) 
            { 
                return mem_ / sizeof(T); 
            }

            void deallocate (pointer p, size_type num) 
            {
            }

        private:
            void    *mem_;
            size_t  size_;
    };

    template <class T1, class T2>
    bool operator== (fixed_allocator <T1> const &a, fixed_allocator <T2> const &b)
    {
        return a.mem_ == b.mem_;
    }

    template <class T1, class T2>
    bool operator!= (fixed_allocator <T1> const &a, fixed_allocator <T2> const &b)
    {
        return a.mem_ != b.mem_;
    }

    template <typename T, size_t N>
    class stack_allocator : public fixed_allocator <T>
    {
        public:
            template <class U> 
            struct rebind { typedef allocator_base<U> other; };

            template <class U> 
            allocator_base (stack_allocator<U> const &r) : 
                mem_ (r.mem_), size_ (r.size_) {}

            stack_allocator () : fixed_allocator (mem_, N) {}
            stack_allocator (stack_allocator const &) = default;
            ~stack_allocator () = default;
        
        private:
            T   mem_[N];
    };

}

#endif
