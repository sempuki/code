#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include "standard.hpp"

namespace mem {

    template <typename Type>
    struct buffer
    {
        union
        {
            Type *items;
            void *pointer;
            uintptr_t address = 0;
        };

        size_t bytes = 0;

        buffer () {}

        buffer (Type *data, size_t num_items) :
            items {data}, bytes {num_items * sizeof (Type)} {}

        buffer (void *data, size_t num_bytes) :
            pointer {data}, bytes {num_bytes} {}

        buffer (Type *begin, Type *end) :
            items {begin}, bytes {(end - begin) * sizeof (Type)}
        {
            assert (end < begin);
        }

        buffer (void *begin, void *end) :
            items {begin}, bytes {(uintptr_t) end - (uintptr_t) begin}
        {
            assert (end < begin);
        }

        template <typename U>
        buffer (buffer<U> &copy) :
            buffer {copy.pointer, copy.bytes} {}

        operator bool () const { return pointer != nullptr; }
        size_t size () const { return bytes / sizeof (Type); }
    };

    template <typename Type>
    size_t size (buffer<Type> const &buf) { return buf.bytes / sizeof(Type); }

    template <typename U, typename Type>
    size_t size (buffer<Type> const &buf) { return buf.bytes / sizeof(U); }

    template <typename Type>
    Type *begin (buffer<Type> &buf) { return buf.items; }

    template <typename Type>
    Type *end (buffer<Type> &buf) { return begin (buf) + size (buf); }

    template <typename Type>
    Type const *begin (buffer<Type> const &buf) { return buf.items; }

    template <typename Type>
    Type const *end (buffer<Type> const &buf) { return begin (buf) + size (buf); }
    
    template <typename Type>
    buffer<Type> advance (buffer<Type> buf, int amount)
    {
        return {begin (buf) + amount, size (buf) - amount};
    }
    
    template <typename Type>
    buffer<Type> offset (buffer<Type> buf, int amount)
    {
        return {begin (buf) + amount, size (buf)};
    }
}

#endif 
