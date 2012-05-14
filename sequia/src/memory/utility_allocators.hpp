#ifndef _UTILITY_ALLOCATORS_HPP_
#define _UTILITY_ALLOCATORS_HPP_

#include <memory/allocator_base.hpp>

namespace sequia
{
    namespace memory
    {
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
    }
}

#endif
