#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <vector>

namespace sequia
{
    template <typename T, size_t N>
    class fixedvector : 
        private stack_identity_allocator<T, N>,
        public std::vector<T, stack_identity_allocator<T, N>>
    {
        public:
            typedef stack_identity_allocator<T, N>  allocator_type; 
            typedef std::vector<T, allocator_type>  container_type;

            explicit fixedvector () : 
                container_type ((allocator_type) *this) {}
            
            explicit fixedvector (size_t n, const T &value = T()) : 
                container_type (n, value, (allocator_type) *this) {}
            
            fixedvector (fixedvector const &copy) : 
                container_type (copy, (allocator_type) *this) {}

            template <class InIterator> 
            fixedvector (InIterator first, InIterator last) : 
                container_type (first, last, (allocator_type) *this) {}
    };
}

#endif
