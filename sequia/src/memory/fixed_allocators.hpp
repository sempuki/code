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
        // Fixed Size Allocators

        //-------------------------------------------------------------------------

        template <typename T, size_t N>
        class fixed_identity_allocator : public identity_allocator<T>
        {
            public:
                typedef identity_allocator<T>       parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                static constexpr size_t mem_size = N * sizeof(value_type);

                template <typename U> 
                struct rebind { typedef fixed_identity_allocator<U, N> other; };

                template <typename U> 
                fixed_identity_allocator (fixed_identity_allocator<U, N> const &r) : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

                fixed_identity_allocator () : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

            private:
                uint8_t  mem_[mem_size]; // uninitialized memory
        };

        //-------------------------------------------------------------------------

        template <typename T, size_t N>
        class fixed_unity_allocator : public unity_allocator<T, typename core::min_word_size<N-1>::type>
        {
            public:
                typedef unity_allocator<T, typename core::min_word_size<N-1>::type> parent_type;
                DECLARE_INHERITED_ALLOCATOR_TYPES_ (parent_type);

                static constexpr size_t mem_size = N * sizeof(value_type);

                template <typename U> 
                struct rebind { typedef fixed_unity_allocator<U, N> other; };

                template <typename U> 
                fixed_unity_allocator (fixed_unity_allocator<U, N> const &r) : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

                fixed_unity_allocator () : 
                    parent_type (reinterpret_cast <pointer> (mem_), N) {}

            private:
                uint8_t  mem_[mem_size]; // uninitialized memory
        };
    }
}

#endif
