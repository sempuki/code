#ifndef _MEMORY_CORE_HPP_
#define _MEMORY_CORE_HPP_


namespace sequia { namespace memory {

    //-------------------------------------------------------------------------

    template <typename T>
    using aligned_storage = typename std::aligned_storage 
        <sizeof(T), std::alignment_of<T>::value>::type;

    template <typename T, size_t Alignment = sizeof(T)>
    inline T* make_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <T *> (p & ~(Alignment-1));
    }

    template <typename T, size_t Alignment = sizeof(T)>
    inline T* next_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <T *> ((p + Alignment-1) & ~(Alignment-1));
    }

    template <typename T, size_t Alignment = sizeof(T)>
    inline size_t aligned_offset (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <size_t> (p & (Alignment-1));
    }

    template <typename T, size_t Alignment = sizeof(T)>
    inline size_t aligned_overhead (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <size_t> ((Alignment - (p & Alignment-1)) & Alignment-1);
    }

    template <typename T, size_t Alignment = sizeof(T)>
    inline bool is_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return !(p & Alignment-1);
    }

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

        explicit buffer (size_t num_items) : 
            items {nullptr}, length {num_items * sizeof (Type)} 
        {}

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

    // TODO: address and space aligned buffers

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

    template <typename U, typename T>
    buffer<U> aligned_buffer (buffer<T> const &buf)
    {
        void const *address = next_aligned <U> (buf.address);
        auto overhead = aligned_overhead <U> (buf.address);
        auto length = (byte_count (buf) > overhead)? byte_count (buf) - overhead : 0;

        return buffer<U> {address, length};
    }

    template <typename U, typename T>
    std::pair<size_t,size_t> aligned_overhead (buffer<T> const &buf)
    {
        size_t pointer_overhead = 0;
        size_t size_overhead = 0;

        auto buf_begin = byte_begin (buf);
        auto buf_end = byte_end (buf);
        auto buf_length = byte_count (buf);

        auto bytes = reinterpret_cast <uint8_t const *> (next_aligned <U> (buf_begin));
        auto length = (buf_end > bytes)? (buf_end - bytes) / sizeof(U) : 0;

        if (length > 0)
        {
            pointer_overhead = bytes - buf_begin;
            size_overhead = buf_length - length;
        }

        return std::make_pair (pointer_overhead, size_overhead);
    }

} } 

#endif
