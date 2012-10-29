#ifndef _MEMORY_CORE_HPP_
#define _MEMORY_CORE_HPP_


namespace sequia
{
    namespace memory
    {
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

            constexpr size_t nitems () { return N; }
            constexpr size_t nbytes () { return N * sizeof(T); }

            constexpr bool valid () { return true; }

            inline bool contains (T *p) { return p >= items && p < items + nitems(); }
            inline bool contains (uint8_t *p) { return p >= bytes && p < bytes + nbytes(); }
        };

        //--------------------------------------------------------------------

        template <typename T>
        struct buffer
        {
            union 
            {
                T       *items;
                uint8_t *bytes;
                void    *memory;
            };

            size_t  size;

            buffer () : 
                memory {nullptr}, size {0} {}

            buffer (size_t num) : 
                items {nullptr}, size {num} {}

            buffer (T *data, size_t num) : 
                items {data}, size {num} {}

            buffer (void *data, size_t bytes) : 
                memory {data}, size {bytes / sizeof(T)} {}

            template <size_t N>
            buffer (static_buffer<T, N> &copy) :
                items {copy.items}, size {N} {}

            buffer &operator= (buffer &r)
            {
                items = r.items;
                size = r.size;
                return *this;
            }

            template <size_t N>
            buffer &operator= (static_buffer<T, N> &r)
            {
                items = r.items;
                size = N;
                return *this;
            }

            inline bool valid () const { items != nullptr; }
            inline void invalidate () { items = nullptr; }

            inline size_t nitems () const { return size; }
            inline size_t nbytes () const { return size * sizeof(T); }

            inline void nitems (size_t n) { size = n; }
            inline void nbytes (size_t n) { size = n * sizeof(T); }

            inline bool contains (T *p) { return p >= items && p < items + nitems(); }
            inline bool contains (uint8_t *p) { return p >= bytes && p < bytes + nbytes(); }
        };

        template <typename T>
        void swap (buffer<T> &a, buffer<T> &b)
        {
            a.memory = b.memory;
            a.size = b.size;
        }

        //--------------------------------------------------------------------

        template <typename T>
        using aligned_storage = typename std::aligned_storage 
            <sizeof(T), std::alignment_of<T>::value>::type;

        template <typename T, size_t Alignment = sizeof(T)>
        inline T* next_aligned (void const *pointer)
        {
            uintptr_t p = reinterpret_cast <uintptr_t> (pointer);
            return reinterpret_cast <T *> ((p + Alignment-1) & ~(Alignment-1));
        }

        template <typename T, size_t Alignment = sizeof(T)>
        inline T* make_aligned (void const *pointer)
        {
            uintptr_t p = reinterpret_cast <uintptr_t> (pointer);
            return reinterpret_cast <T *> (p & ~(Alignment-1));
        }

        template <typename T, size_t Alignment = sizeof(T)>
        inline size_t aligned_difference (void const *pointer)
        {
            uintptr_t p = reinterpret_cast <uintptr_t> (pointer);
            return Alignment - (p & Alignment-1);
        }
    }
}

#endif
