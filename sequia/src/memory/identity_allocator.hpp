#ifndef _IDENTITY_ALLOCATOR_HPP_
#define _IDENTITY_ALLOCATOR_HPP_

#include <memory/stateful_allocator_base.hpp>

namespace sequia
{
    namespace memory
    {
        //----------------------------------------------------------------------
        // Only allocates the same buffer for each call

        template <typename T>
        class identity_allocator : public stateful_allocator_base<T>
        {
            public:
                typedef identity_allocator<T>       this_type;
                typedef stateful_allocator_base<T>  parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                template <typename U> 
                struct rebind { typedef identity_allocator<U> other; };

                template <typename U> 
                identity_allocator (identity_allocator<U> const &r) : 
                    parent_type {r} {}

                identity_allocator (void *p, size_type s) : 
                    parent_type {p, s} {}

                size_type max_size () const 
                { 
                    return buffer<T>::size;
                }

                pointer allocate (size_type num, const void* = 0) 
                { 
                    return buffer<T>::mem; 
                }

                void deallocate (pointer ptr, size_type num) 
                {
                }

                static size_type calc_size (size_type n)
                {
                    return sizeof(this_type) + sizeof(T) * n;
                }
        };
    }
}

#endif
