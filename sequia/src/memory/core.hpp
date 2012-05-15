#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_

namespace sequia
{
    namespace memory
    {
        template <typename T>
        struct buffer
        {
            size_t  size;   
            T       *mem;

            buffer () : 
                size {0}, 
                mem {0} {}

            buffer (size_t s, T *m) : 
                size (s), 
                mem (m) {}

            buffer (size_t s, void *m) : 
                size {s / sizeof(T)}, 
                mem {static_cast<T *>(m)} {}
        };
    }
}

#endif
