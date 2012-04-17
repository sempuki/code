#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <vector>
#include <map>

namespace sequia
{
    template <size_t N, typename T>
    class fixedvector : 
        private stack_identity_allocator <N, T>,
        public std::vector <T, stack_identity_allocator <N, T>>
    {
        public:
            typedef stack_identity_allocator <N, T> allocator_type; 
            typedef std::vector <T, allocator_type> container_type;

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

    template <size_t N, typename K, typename V, typename Compare = std::less<K>>
    class fixedmap : 
        private stack_unity_allocator <N, std::pair <const K, V>>,
        public std::map<K, V, Compare, stack_unity_allocator <N, std::pair <const K, V>>>
    {
        public:
            typedef stack_unity_allocator <N, std::pair <const K, V>>   allocator_type; 
            typedef std::map <K, V, Compare, allocator_type>            container_type;

            explicit fixedmap (Compare const &comp = Compare()) :
                container_type (comp, (allocator_type) *this) {}

            fixedmap (fixedmap const &copy) :
                container_type (copy, (allocator_type) *this) {}

            template <class InIterator> 
            fixedmap (InIterator first, InIterator last, Compare const & comp = Compare()) :
                container_type (first, last, comp, (allocator_type) *this) {}

    };
}

#endif
