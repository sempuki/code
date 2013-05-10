#ifndef _MEMORY_CORE_HPP_
#define _MEMORY_CORE_HPP_


namespace sequia { namespace memory {

    //-------------------------------------------------------------------------

    template <typename Type, size_t N>
    struct static_buffer
    {
        Type items [N];

        static_buffer () {}
        operator bool () const { return true; }
        size_t size () const { return N; }
    };

    //-------------------------------------------------------------------------

    template <typename Type>
    struct buffer
    {
        union 
        {
            Type *const items = nullptr;
            void const *const address;
        };

        size_t const bytes = 0;

        buffer () {}

        buffer (Type *data, size_t num_items) : 
            items {data}, bytes {num_items * sizeof (Type)} {}

        buffer (Type *begin, Type *end) :
            items {begin}, bytes {(end - begin) * sizeof (Type)} 
        {
            ASSERTF (end < begin, "invalid range passed");
        }

        buffer (void const *data, size_t num_bytes) : 
            address {data}, bytes {num_bytes} {}

        template <typename U>
        buffer (buffer<U> const &copy) :
            buffer {copy.address, copy.bytes} {}

        template <typename U, size_t N>
        buffer (static_buffer<U, N> &copy) :
            address {copy.address, N / sizeof (U)} {}

        operator bool () const { return address != nullptr && bytes != 0; }
        size_t size () const { return bytes / sizeof (Type); }
        
        void reset (buffer<Type> const &other = {})
        { 
            const_cast<void const *&> (address) = other.address; 
            const_cast<size_t &> (bytes) = other.bytes; 
        }
    };

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    size_t count (static_buffer<T,N> const &buf) { return N; }

    template <typename U, typename T, size_t N>
    size_t count (static_buffer<T,N> const &buf) { return (N * sizeof(T)) / sizeof(U); }

    template <typename T, size_t N>
    size_t count_bytes (static_buffer<T,N> const &buf) { return N * sizeof(T); }

    template <typename T>
    size_t count (buffer<T> const &buf) { return buf.bytes / sizeof(T); }

    template <typename U, typename T>
    size_t count (buffer<T> const &buf) { return buf.bytes / sizeof(U); }

    template <typename T>
    size_t count_bytes (buffer<T> const &buf) { return buf.bytes; }

    //-------------------------------------------------------------------------

    template <typename T, size_t N>
    T *begin (static_buffer<T,N> &buf) { return buf.items; }

    template <typename T, size_t N>
    T *end (static_buffer<T,N> &buf) { return begin (buf) + count (buf); }

    template <typename T, size_t N>
    T const *begin (static_buffer<T,N> const &buf) { return buf.items; }

    template <typename T, size_t N>
    T const *end (static_buffer<T,N> const &buf) { return begin (buf) + count (buf); }

    template <typename T>
    T *begin (buffer<T> &buf) { return buf.items; }

    template <typename T>
    T *end (buffer<T> &buf) { return begin (buf) + count (buf); }

    template <typename T>
    T const *begin (buffer<T> const &buf) { return buf.items; }

    template <typename T>
    T const *end (buffer<T> const &buf) { return begin (buf) + count (buf); }

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
        return buffer<T> (buf.items, 1 << core::bit::log2_floor (buf.size ()));
    }

    template <typename U, typename T>
    buffer<U> make_aligned_buffer (buffer<T> const &buf, uintptr_t alignment)
    {
        auto address = next_aligned<void const *> (buf.address, alignment);
        auto overhead = aligned_overhead (buf.address, alignment);
        auto bytes = (count_bytes (buf) > overhead)? count_bytes (buf) - overhead : 0;

        return buffer<U> (address, bytes);
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
