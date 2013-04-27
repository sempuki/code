#ifndef _MEMORY_CORE_HPP_
#define _MEMORY_CORE_HPP_


namespace sequia { namespace memory {

    //-------------------------------------------------------------------------

    template <typename Type, size_t N>
    struct static_buffer
    {
        union 
        {
            Type    items [N];
            uint8_t bytes [N * sizeof(Type)];
        };

        static_buffer () {}
        operator bool () const { return true; }
    };

    //-------------------------------------------------------------------------

    template <typename Type>
    struct buffer
    {
        union 
        {
            Type        *items;
            uint8_t     *bytes;
            void const  *address = nullptr;
        };

        size_t const length = 0;

        buffer (Type *data, size_t num_items) : 
            items {data}, length {num_items * sizeof (Type)} 
        {}

        buffer (Type *begin, Type *end) :
            items {begin}, length {(end - begin) * sizeof (Type)} 
        {
            ASSERTF (end < begin, "invalid range passed");
        }

        buffer (void const *data, size_t num_bytes) : 
            address {data}, length {num_bytes} 
        {}

        template <typename U>
        buffer (buffer<U> const &copy) :
            buffer {copy.address, copy.length} {}

        template <typename U, size_t N>
        buffer (static_buffer<U, N> &copy) :
            address {copy.address, N / sizeof (U)} {}

        operator bool () const { return address != nullptr; }

        void invalidate () { address = nullptr; }
    };

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    size_t item_count (static_buffer<T,N> const &buf) { return N; }

    template <typename T, size_t N>
    size_t byte_count (static_buffer<T,N> const &buf) { return N * sizeof(T); }

    template <typename T>
    size_t item_count (buffer<T> const &buf) { return buf.length / sizeof(T); }

    template <typename T>
    size_t byte_count (buffer<T> const &buf) { return buf.length; }

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    T *begin (static_buffer<T,N> &buf) { return buf.items; }

    template <typename T, size_t N>
    T *end (static_buffer<T,N> &buf) { return buf.items + item_count (buf); }

    template <typename T, size_t N>
    T const *begin (static_buffer<T,N> const &buf) { return buf.items; }

    template <typename T, size_t N>
    T const *end (static_buffer<T,N> const &buf) { return buf.items + item_count (buf); }

    template <typename T>
    T *begin (buffer<T> &buf) { return buf.items; }

    template <typename T>
    T *end (buffer<T> &buf) { return buf.items + item_count (buf); }

    template <typename T>
    T const *begin (buffer<T> const &buf) { return buf.items; }

    template <typename T>
    T const *end (buffer<T> const &buf) { return buf.items + item_count (buf); }

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    T *byte_begin (static_buffer<T,N> &buf) { return buf.bytes; }

    template <typename T, size_t N>
    T *byte_end (static_buffer<T,N> &buf) { return buf.bytes + byte_count (buf); }

    template <typename T, size_t N>
    T const *byte_begin (static_buffer<T,N> const &buf) { return buf.items; }

    template <typename T, size_t N>
    T const *byte_end (static_buffer<T,N> const &buf) { return buf.bytes + byte_count (buf); }

    template <typename T>
    T *byte_begin (buffer<T> &buf) { return buf.items; }

    template <typename T>
    T *byte_end (buffer<T> &buf) { return buf.items + byte_count (buf); }

    template <typename T>
    T const *byte_begin (buffer<T> const &buf) { return buf.items; }

    template <typename T>
    T const *byte_end (buffer<T> const &buf) { return buf.items + byte_count (buf); }

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    bool contains (static_buffer<T,N> const &buf, T *pointer) 
    { 
        return pointer >= begin (buf) && pointer < end (buf); 
    }

    template <typename T>
    bool contains (buffer<T> const &buf, T *pointer) 
    { 
        return pointer >= begin (buf) && pointer < end (buf); 
    }
    
    //-------------------------------------------------------------------------
                
    template <typename T>
    using aligned_storage = typename std::aligned_storage 
        <sizeof(T), std::alignment_of<T>::value>::type;

    template <typename T, uintptr_t Alignment = sizeof(T)>
    inline T* make_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <T *> (p & ~(Alignment-1));
    }

    template <typename T, uintptr_t Alignment = sizeof(T)>
    inline T* next_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <T *> ((p + Alignment-1) & ~(Alignment-1));
    }

    inline size_t aligned_offset (void const *pointer, uintptr_t alignment)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <size_t> (p & (alignment-1));
    }

    template <typename T>
    inline size_t aligned_offset (void const *pointer)
    {
        return aligned_offset (pointer, sizeof(T));
    }

    inline size_t aligned_overhead (void const *pointer, uintptr_t alignment)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <size_t> ((alignment - (p & alignment-1)) & alignment-1);
    }

    template <typename T>
    inline size_t aligned_overhead (void const *pointer)
    {
        return aligned_overhead (pointer, sizeof(T)); 
    }

    inline bool is_aligned (void const *pointer, uintptr_t alignment)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return !(p & alignment-1);
    }

    template <typename T>
    inline bool is_aligned (void const *pointer)
    {
        return is_aligned (pointer, sizeof(T));
    }

    //-------------------------------------------------------------------------

    template <typename T>
    buffer<T> make_pow2_size_buffer (buffer<T> const &buf)
    {
        return buffer<T> (buf.items, 1 << core::bit::log2_floor (item_count (buf)));
    }

    template <typename U, typename T>
    buffer<U> make_aligned_buffer (buffer<T> const &buf, uintptr_t alignment)
    {
        auto address = next_aligned<void const *> (buf.address, alignment);
        auto overhead = aligned_overhead (buf.address, alignment);
        auto length = (byte_count (buf) > overhead)? byte_count (buf) - overhead : 0;

        return buffer<U> (address, length);
    }

    //-------------------------------------------------------------------------

    template <typename Type, uintptr_t Alignment>
    struct aligned_buffer : public buffer<Type>
    {
        aligned_buffer (Type *data, size_t num_items) : 
            buffer<Type> {make_aligned_buffer (buffer<Type> (data, num_items), Alignment)} {}

        aligned_buffer (Type *begin, Type *end) :
            buffer<Type> {make_aligned_buffer (buffer<Type> (begin, end), Alignment)} {}

        aligned_buffer (void const *data, size_t num_bytes) : 
            buffer<Type> {make_aligned_buffer (buffer<Type> (data, num_bytes), Alignment)} {}

        template <typename U>
        aligned_buffer (buffer<U> const &copy) :
            buffer<Type> {make_aligned_buffer (buffer<Type> (copy), Alignment)} {}

        template <typename U, size_t N>
        aligned_buffer (static_buffer<U, N> &copy) :
            buffer<Type> {make_aligned_buffer (buffer<Type> (copy), Alignment)} {}
    };

    template <typename Type>
    struct pow2_size_buffer : public buffer<Type>
    {
        pow2_size_buffer (Type *data, size_t num_items) : 
            buffer<Type> {make_pow2_size_buffer (buffer<Type> (data, num_items))} {}

        pow2_size_buffer (Type *begin, Type *end) :
            buffer<Type> {make_pow2_size_buffer (buffer<Type> (begin, end))} {}

        pow2_size_buffer (void const *data, size_t num_bytes) : 
            buffer<Type> {make_pow2_size_buffer (buffer<Type> (data, num_bytes))} {}

        template <typename U>
        pow2_size_buffer (buffer<U> const &copy) :
            buffer<Type> {make_pow2_size_buffer (buffer<Type> (copy))} {}

        template <typename U, size_t N>
        pow2_size_buffer (static_buffer<U, N> &copy) :
            buffer<Type> {make_pow2_size_buffer (buffer<Type> (copy))} {}
    };

} } 

#endif
