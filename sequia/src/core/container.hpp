#ifndef _CONTAINER_HPP_
#define _CONTAINER_HPP_

#include <memory/allocator/core.hpp>
#include <memory/allocator/constant.hpp>
#include <memory/allocator/identity.hpp>
#include <memory/allocator/scoped.hpp>
#include <memory/allocator/fixed_buffer.hpp>

#define DECLARE_INHERITED_CONTAINER_TYPES(ContainerType) \
    typedef typename ContainerType::value_type              value_type; \
    typedef typename ContainerType::pointer                 pointer; \
    typedef typename ContainerType::const_pointer           const_pointer; \
    typedef typename ContainerType::reference               reference; \
    typedef typename ContainerType::const_reference         const_reference; \
    typedef typename ContainerType::size_type               size_type; \
    typedef typename ContainerType::difference_type         difference_type; \
    typedef typename ContainerType::allocator_type          allocator_type; \
    typedef typename ContainerType::iterator                iterator; \
    typedef typename ContainerType::const_iterator          const_iterator; \
    typedef typename ContainerType::reverse_iterator        reverse_iterator; \
    typedef typename ContainerType::const_reverse_iterator  const_reverse_iterator;


namespace sequia
{
    namespace core
    {
        template <typename T, size_t N>
        using fixed_vector_allocator = 
            memory::allocator::constant<N, 
                memory::allocator::identity<
                    memory::allocator::scoped<
                        memory::allocator::fixed_buffer<N, T>>>>;

        template <typename T, size_t N>
        using fixed_vector = std::vector<T, fixed_vector_allocator<T, N>>;

        //template <typename K, typename V>
        //using fixedmap_allocator = 
        //memory::rebind_allocator<memory::unity_allocator<std::pair<const K, V>, uint16_t>>;

        //template <typename K, typename V, typename Compare = std::less<K>>
        //class fixedmap : public std::map<K, V, Compare, fixedmap_allocator<K, V>>
        //{
        //    public:
        //        typedef std::map<K, V, Compare, fixedmap_allocator<K, V>> parent_type;
        //        DECLARE_INHERITED_CONTAINER_TYPES_ (parent_type);

        //        explicit fixedmap (size_type n) // hack: work around lack of move constructor
        //            : parent_type {allocator_type {nullptr, n}} {}

        //        // TODO: inheriting constructors
        //};

        //template <typename K, typename V, size_t N>
        //using staticmap_allocator = 
        //memory::rebind_allocator<memory::unity_allocator<std::pair<const K, V>, uint16_t, 
        //    static_allocator<std::pair<const K, V>, N>>>;

        //template <typename K, typename V, size_t N, typename Compare = std::less<K>>
        //class staticmap : public std::map<K, V, Compare, staticmap_allocator<K, V, N>>
        //{
        //    public:
        //        typedef std::map<K, V, Compare, staticmap_allocator<K, V, N>> parent_type;
        //        DECLARE_INHERITED_CONTAINER_TYPES_ (parent_type);

        //        staticmap () : parent_type {allocator_type {N}} {}

        //        // TODO: inheriting constructors
        //};
    }
}

#endif
