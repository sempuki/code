#ifndef _MEMORY_CORE_HPP_
#define _MEMORY_CORE_HPP_


namespace sequia
{
    namespace memory
    {
        //--------------------------------------------------------------------

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

        //--------------------------------------------------------------------

        template <typename T, size_t N>
        struct static_buffer
        {
            union 
            {
                T       items [N];
                uint8_t bytes [N * sizeof(T)];
            };

            static_buffer () {}

            constexpr bool valid () { return true; }

            constexpr size_t nitems () { return N; }
            constexpr size_t nbytes () { return N * sizeof(T); }

            inline T *begin () { return items; }
            inline T *end () { return items + N; }

            inline uint8_t *byte_begin () { return bytes; }
            inline uint8_t *byte_end () { return bytes + N * sizeof(T); }

            inline bool contains (T *p) const { return p >= items && p < items + N; }
        };

        //--------------------------------------------------------------------

        template <typename T>
        struct buffer
        {
            union 
            {
                T           *items;
                uint8_t     *bytes;
                void const  *address;
            };

            size_t  size;

            buffer () : 
                address {nullptr}, size {0} {}

            buffer (size_t num) : 
                address {nullptr}, size {num} {}

            buffer (T *data, size_t num) : 
                items {data}, size {num} {}

            buffer (void const *data, size_t bytes) : 
                address {data}, size {bytes / sizeof(T)} 
            {
                std::cout << "bytes: " << bytes << std::endl;
                std::cout << "size: " << size << std::endl;
                std::cout << "sizeof(T): " << sizeof(T) << std::endl;
                CONFIRMF (bytes == size * sizeof(T), 
                        "lost %ld bytes from buffer conversion", bytes - (size * sizeof(T)));
            }

            template <size_t N>
            buffer (static_buffer<T, N> &copy) :
                address {copy.address}, size {N} {}

            buffer &operator= (buffer &r)
            {
                address = r.address;
                size = r.size;
                return *this;
            }

            template <size_t N>
            buffer &operator= (static_buffer<T, N> &r)
            {
                address = r.address;
                size = N;
                return *this;
            }

            inline bool valid () const { return address != nullptr; }
            inline void invalidate () { address = nullptr; }

            inline size_t nitems () const { return size; }
            inline size_t nbytes () const { return size * sizeof(T); }

            inline T *begin () { return items; }
            inline T *end () { return items + size; }

            inline uint8_t *byte_begin () { return bytes; }
            inline uint8_t *byte_end () { return bytes + size * sizeof(T); }

            inline bool contains (T *p) const { return p >= items && p < items + size; }
        };

        template <typename T>
        void swap (buffer<T> &a, buffer<T> &b)
        {
            swap (a.address, b.address);
            swap (a.size, b.size);
        }

        template <typename U, typename T>
        buffer<U> align (buffer<T> const &buf)
        {
            void const *address = next_aligned<U> (buf.address);
            size_t size = buf.nbytes() - aligned_overhead<U> (buf.address);

            return buffer<U> {address, size};
        }
    }
}

#endif
