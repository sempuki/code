#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <vector>
#include <map>

namespace sequia
{
    template <size_t N, typename T>
    class fixedvector : 
        public std::vector <T, fixed_identity_allocator <N, T>>
    {
    };

    template <size_t N, typename K, typename V, typename Compare = std::less<K>>
    class fixedmap : 
        public std::map<K, V, Compare, rebind_allocator <std::pair<const K, V>, fixed_unity_allocator <N, std::pair <const K, V>>>>
    {
    };
}

#endif