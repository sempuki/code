#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <vector>
#include <map>

namespace sequia
{
    // GCC 4.7 TODO template aliases
    
    template <typename T, size_t N>
    class fixedvector : 
        public std::vector<T, fixed_identity_allocator<T, N>>
    {
    };

    template <typename K, typename V, size_t N, typename Compare = std::less<K>>
    class fixedmap : 
        public std::map<K, V, Compare, rebind_allocator<fixed_unity_allocator<std::pair<const K, V>, N>>>
    {
    };
}

#endif
