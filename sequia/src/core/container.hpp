#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <memory/allocator/core.hpp>
#include <memory/allocator/compat.hpp>
#include <memory/allocator/constant.hpp>
#include <memory/allocator/identity.hpp>
#include <memory/allocator/unity.hpp>
#include <memory/allocator/scoped.hpp>
#include <memory/allocator/fixed_buffer.hpp>


namespace sequia
{
    namespace core
    {
        template <size_t N, typename T>
        using fixed_vector_allocator = 
            memory::allocator::constant< 
                memory::allocator::concrete<
                    memory::allocator::identity<
                        memory::allocator::scoped<
                            memory::allocator::fixed_buffer<N, T>>>>, N>;

        template <size_t N, typename T>
        using fixed_vector = std::vector<T, fixed_vector_allocator<N, T>>;


        template <size_t N, typename K, typename V>
        using fixed_map_allocator = 
            memory::allocator::compat< 
                memory::allocator::constant<
                    memory::allocator::concrete<
                        memory::allocator::unity<
                            memory::allocator::scoped<
                                memory::allocator::fixed_buffer<N, std::pair<const K, V>>>, 
                        typename core::min_word_size<N-1>::type>, N>>>;

        template <size_t N, typename K, typename V, typename Compare = std::less<K>>
        using fixed_map = std::map<K, V, Compare, fixed_map_allocator<N, K, V>>;
    }
}

#endif
