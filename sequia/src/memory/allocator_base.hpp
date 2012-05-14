#ifndef _ALLOCATOR_BASE_HPP_
#define _ALLOCATOR_BASE_HPP_

#include <memory/core.hpp>

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

        //::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
        
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
    }
}

#endif
