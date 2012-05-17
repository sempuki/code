#ifndef _STATEFUL_ALLOCATOR_BASE_HPP_
#define _STATEFUL_ALLOCATOR_BASE_HPP_

#include <memory/allocator_base.hpp>

namespace sequia
{
    namespace memory
    {
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

                static size_type calc_size (pointer p, size_type n);
        };

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
        
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
    }
}

#endif
