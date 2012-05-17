#ifndef _FIXED_ALLOCATOR_HPP_
#define _FIXED_ALLOCATOR_HPP_

#include <core/types.hpp>

#include <memory/identity_allocator.hpp>
#include <memory/unity_allocator.hpp>

namespace sequia
{
    namespace memory
    {
        //=========================================================================
        // Static Allocators

        //-------------------------------------------------------------------------

        template <typename T, size_t N>
        class static_identity_allocator : public identity_allocator<T>
        {
            public:
                typedef static_identity_allocator<T, N> this_type;
                typedef identity_allocator<T>       parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                static constexpr size_t mem_size = N * sizeof(value_type);

                template <typename U> 
                struct rebind { typedef static_identity_allocator<U, N> other; };

                template <typename U> 
                static_identity_allocator (static_identity_allocator<U, N> const &r) : 
                    parent_type {reinterpret_cast <pointer> (mem_), N} {}

                static_identity_allocator () : 
                    parent_type {reinterpret_cast <pointer> (mem_), N} {}

                static size_type calc_size () { return sizeof(this_type); }

            private:
                uint8_t  mem_[mem_size]; // uninitialized memory
        };

        //-------------------------------------------------------------------------

        template <typename T, size_t N>
        class static_unity_allocator : public unity_allocator<T, typename core::min_word_size<N-1>::type>
        {
            public:
                typedef static_unity_allocator<T, N> this_type;
                typedef unity_allocator<T, typename core::min_word_size<N-1>::type> parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                static constexpr size_t mem_size = N * sizeof(value_type);

                template <typename U> 
                struct rebind { typedef static_unity_allocator<U, N> other; };

                template <typename U> 
                static_unity_allocator (static_unity_allocator<U, N> const &r) : 
                    parent_type {reinterpret_cast <pointer> (mem_), N} {}

                static_unity_allocator () : 
                    parent_type {reinterpret_cast <pointer> (mem_), N} {}

                static size_type calc_size () { return sizeof(this_type); }

            private:
                uint8_t  mem_[mem_size]; // uninitialized memory
        };
    }
}

#endif
