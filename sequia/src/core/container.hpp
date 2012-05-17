#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <vector>
#include <map>

#include <memory/static_allocators.hpp>
#include <memory/utility_allocators.hpp>

namespace sequia
{
    namespace core
    {
        template <typename T>
        using fixedvector = std::vector<T, memory::identity_allocator<T>>;
    
        template <typename T, size_t N>
        using staticvector = std::vector<T, memory::static_identity_allocator<T, N>>;

        template <typename K, typename V, typename Compare = std::less<K>>
        using fixedmap = std::map<K, V, Compare, 
              memory::rebind_allocator<memory::unity_allocator<std::pair<const K, V>, uint16_t>>>;

        template <typename K, typename V, size_t N, typename Compare = std::less<K>>
        using staticmap = std::map<K, V, Compare, 
              memory::rebind_allocator<memory::static_unity_allocator<std::pair<const K, V>, N>>>;
    }
}

#endif
