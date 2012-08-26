#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <memory/allocator/core.hpp>
#include <memory/allocator/stateful.hpp>
#include <memory/allocator/terminal.hpp>
#include <memory/allocator/fixed_buffer.hpp>
#include <memory/allocator/identity.hpp>
#include <memory/allocator/unity.hpp>
#include <memory/allocator/compat.hpp>
#include <memory/allocator/scoped.hpp>
#include <memory/allocator/concrete.hpp>
//#include <memory/allocator/constant.hpp>

namespace sequia
{
    namespace core
    {
        template <typename T, size_t N>
        using fixed_vector_allocator = 
            memory::allocator::concrete<
                memory::allocator::identity<
                    memory::allocator::scoped<
                        memory::allocator::fixed_buffer<N>>>, T>;

        template <typename T, size_t N>
        using fixed_vector = std::vector<T, fixed_vector_allocator<T, N>>;

        template <size_t N, typename K, typename V>
        using fixed_map_allocator = 
            memory::allocator::concrete<
                memory::allocator::compat<
                    memory::allocator::unity<
                        memory::allocator::scoped<
                            memory::allocator::fixed_buffer<N>>, 
                        typename core::min_word_size<N-1>::type>>, 
                std::pair<const K, V>>;

        template <size_t N, typename K, typename V, typename Compare = std::less<K>>
        using fixed_map = std::map<K, V, Compare, fixed_map_allocator<N, K, V>>;
    }
}

#endif
