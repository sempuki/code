#ifndef MEMORY_CORE_HPP_
#define MEMORY_CORE_HPP_


namespace sequia { namespace memory {

    // bit buffer ============================================================
    
    struct bitbuffer
    {
        uint8_t *base = nullptr;
        uint32_t limit = 0;
        uint32_t offset = 0;

        bitbuffer () = default;

        bitbuffer (uint8_t *data, size_t size, size_t begin = 0) : 
            base {data}, limit (size), offset (begin) {}

        bitbuffer (uint8_t *begin, uint8_t *end) : 
            base {begin}, limit (((uintptr_t) end - (uintptr_t) begin) * 8) {}
    };

    size_t size (bitbuffer const &buf)
    {
        return buf.limit - buf.offset;
    }

    // static buffer ==========================================================

    template <typename Type, size_t N>
    struct static_buffer
    {
        Type items [N];

        static_buffer () {}
        operator bool () const { return true; }
        size_t size () const { return N; }
    };

    // buffer =================================================================

    template <typename Type>
    struct buffer
    {
        union 
        {
            Type *const items;
            void const *const pointer;
            uintptr_t address = 0;
        };

        size_t const bytes = 0;

        buffer () {}

        buffer (Type *data, size_t num_items) : 
            items {data}, bytes {num_items * sizeof (Type)} {}

        buffer (void const *data, size_t num_bytes) : 
            pointer {data}, bytes {num_bytes} {}

        buffer (Type *begin, Type *end) :
            items {begin}, bytes {(end - begin) * sizeof (Type)} 
        {
            ASSERTF (end < begin, "invalid range passed");
        }

        buffer (void const *begin, void const *end) :
            items {begin}, bytes {(uintptr_t) end - (uintptr_t) begin} 
        {
            ASSERTF (end < begin, "invalid range passed");
        }

        template <typename U>
        buffer (buffer<U> const &copy) :
            buffer {copy.pointer, copy.bytes} {}

        template <typename U, size_t N>
        buffer (static_buffer<U, N> const &copy) :
            pointer {copy.pointer, N / sizeof (U)} {}

        operator bool () const { return pointer != nullptr; }
        size_t size () const { return bytes / sizeof (Type); }
        
        buffer &reset (buffer<Type> const &other = {})
        { 
            const_cast<void const *&> (pointer) = other.pointer; 
            const_cast<size_t &> (bytes) = other.bytes; 
            return *this;
        }
    };

    // byte buffer ===========================================================
    
    using bytebuffer = buffer<uint8_t>;

    // buffer size ------------------------------------------------------------

    template <typename Type>
    size_t size (buffer<Type> const &buf) { return buf.bytes / sizeof(Type); }

    template <typename U, typename Type>
    size_t size (buffer<Type> const &buf) { return buf.bytes / sizeof(U); }

    template <typename Type, size_t N>
    size_t size (static_buffer<Type,N> const &buf) { return N; }

    template <typename U, typename Type, size_t N>
    size_t size (static_buffer<Type,N> const &buf) { return (N * sizeof(Type)) / sizeof(U); }

    // buffer iterators -------------------------------------------------------

    template <typename Type, size_t N>
    Type *begin (static_buffer<Type,N> &buf) { return buf.items; }

    template <typename Type, size_t N>
    Type *end (static_buffer<Type,N> &buf) { return begin (buf) + size (buf); }

    template <typename Type, size_t N>
    Type const *begin (static_buffer<Type,N> const &buf) { return buf.items; }

    template <typename Type, size_t N>
    Type const *end (static_buffer<Type,N> const &buf) { return begin (buf) + size (buf); }

    template <typename Type>
    Type *begin (buffer<Type> &buf) { return buf.items; }

    template <typename Type>
    Type *end (buffer<Type> &buf) { return begin (buf) + size (buf); }

    template <typename Type>
    Type const *begin (buffer<Type> const &buf) { return buf.items; }

    template <typename Type>
    Type const *end (buffer<Type> const &buf) { return begin (buf) + size (buf); }

    // buffer operations ------------------------------------------------------

    template <typename Type, size_t N>
    bool contains (static_buffer<Type,N> const &buf, Type *pointer) 
    { 
        return pointer >= begin (buf) && pointer < end (buf); 
    }

    template <typename Type>
    bool contains (buffer<Type> const &buf, Type *pointer) 
    { 
        return pointer >= begin (buf) && pointer < end (buf); 
    }
    
    template <typename Type>
    buffer<Type> grow (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf), size (buf) + amount};
    }

    template <typename Type>
    buffer<Type> shrink (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf), size (buf) - amount};
    }

    template <typename Type>
    buffer<Type> advance (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf) + amount, size (buf)};
    }

    template <typename Type>
    buffer<Type> retreat (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf) - amount, size (buf)};
    }

    template <typename Type>
    buffer<Type> offset (buffer<Type> &buf, int amount)
    {
        return {begin (buf) + amount, size (buf) - amount};
    }

    template <typename Type, typename U>
    bool preceeds (buffer<Type> const &a, buffer<U> const &b)
    {
        return a.address < b.address;
    }

    template <typename Type, typename U>
    bool smaller (buffer<Type> const &a, buffer<U> const &b)
    {
        return a.bytes < b.bytes;
    }

    // alignment ==============================================================
                
    template <typename Type>
    using aligned_storage = typename std::aligned_storage 
        <sizeof(Type), std::alignment_of<Type>::value>::type;

    template <typename Type, uintptr_t Alignment = sizeof(Type)>
    inline Type* make_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <Type *> (p & ~(Alignment-1));
    }

    template <typename Type, uintptr_t Alignment = sizeof(Type)>
    inline Type* next_aligned (void const *pointer)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <Type *> ((p + Alignment-1) & ~(Alignment-1));
    }

    inline size_t aligned_offset (void const *pointer, uintptr_t alignment)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <size_t> (p & (alignment-1));
    }

    template <typename Type>
    inline size_t aligned_offset (void const *pointer)
    {
        return aligned_offset (pointer, sizeof(Type));
    }

    inline size_t aligned_overhead (void const *pointer, uintptr_t alignment)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return reinterpret_cast <size_t> ((alignment - (p & alignment-1)) & alignment-1);
    }

    template <typename Type>
    inline size_t aligned_overhead (void const *pointer)
    {
        return aligned_overhead (pointer, sizeof(Type)); 
    }

    inline bool is_aligned (void const *pointer, uintptr_t alignment)
    {
        auto p = reinterpret_cast <uintptr_t const> (pointer);
        return !(p & alignment-1);
    }

    template <typename Type>
    inline bool is_aligned (void const *pointer)
    {
        return is_aligned (pointer, sizeof(Type));
    }

    // aligned buffer =========================================================

    template <typename U, typename Type>
    buffer<U> make_aligned_buffer (buffer<Type> const &buf, uintptr_t alignment)
    {
        auto pointer = next_aligned<void const *> (buf.pointer, alignment);
        auto overhead = aligned_overhead (buf.pointer, alignment);
        auto bytes = (count_bytes (buf) > overhead)? count_bytes (buf) - overhead : 0;

        return {pointer, bytes};
    }

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

    // power size buffer ======================================================
    
    template <typename Type>
    buffer<Type> make_pow2_size_buffer (buffer<Type> const &buf)
    {
        return {buf.items, 1 << core::bit::log2_floor (buf.size ())};
    }

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
