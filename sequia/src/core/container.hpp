#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <memory/allocator/static_buffer.hpp>
#include <memory/allocator/static_item.hpp>

namespace sequia
{
    namespace core
    {
        template <typename T, size_t N>
        using static_vector = std::vector<T, 
              memory::allocator::static_buffer<T, N>>;

        template <typename K, typename V, size_t N, typename Compare = std::less<K>>
        using static_map = std::map<K, V, Compare, 
              memory::allocator::static_item<typename std::map<K,V,Compare>::value_type, N>>;
    }
}

#endif
