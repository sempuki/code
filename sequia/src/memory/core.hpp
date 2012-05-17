#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

namespace sequia
{
    namespace memory
    {
        template <typename T>
        struct buffer
        {
            T       *mem;
            size_t  size;   

            buffer () : 
                mem {0},
                size {0} {}

            buffer (T *m, size_t s) : 
                mem {m},
                size {s} {}

            buffer (void *m, size_t s) : 
                mem {reinterpret_cast <T *> (m)},
                size {s / sizeof(T)} {}
        };

        template <typename T, typename U>
        auto reserve (void *ptr, size_t space) -> T *
        {
            return (T *) ((uint8_t *) (ptr) + (sizeof(U) * space));
        }
    }
}

#endif
