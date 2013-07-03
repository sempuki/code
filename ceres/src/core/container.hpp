#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <memory/allocator/static_buffer.hpp>
#include <memory/allocator/static_item.hpp>
#include <memory/allocator/fixed_buffer.hpp>
#include <memory/allocator/fixed_item.hpp>

namespace ceres { namespace core {

    // TODO: specialize allocator for vector<bool>

    template <typename T, size_t N>
    using static_vector = std::vector<T, 
          memory::allocator::static_buffer<typename std::vector<T>::value_type, N>>;

    template <typename T>
    using fixed_vector = std::vector<T, 
          memory::allocator::fixed_buffer<typename std::vector<T>::value_type>>;

    template <typename T>
    fixed_vector<T> make_fixed_vector (size_t N)
    {
        return fixed_vector<T> {typename fixed_vector<T>::allocator_type {N}};
    }

    template <typename T>
    fixed_vector<T> make_allocated_fixed_vector (size_t N)
    {
        return fixed_vector<T> {N, T(), 
            typename fixed_vector<T>::allocator_type {N}};
    }

    template <typename K, typename V, size_t N, typename Compare = std::less<K>>
    using static_map = std::map<K, V, Compare, 
          memory::allocator::static_item<typename std::map<K,V,Compare>::value_type, N>>;
    
    template <typename K, typename V, typename Compare = std::less<K>>
    using fixed_map = std::map<K, V, Compare, 
          memory::allocator::fixed_item<typename std::map<K,V,Compare>::value_type>>;

    template <typename K, typename V, typename Compare = std::less<K>>
    fixed_map<K,V,Compare> make_fixed_map (size_t N)
    {
        return fixed_map<K,V,Compare> {Compare(), // TODO: std::map not fully c++11'ed
            typename fixed_map<K,V,Compare>::allocator_type {N}};
    }

} }

#endif
