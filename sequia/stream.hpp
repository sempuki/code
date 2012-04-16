#ifndef _STREAM_HPP_
#define _STREAM_HPP_

namespace traits
{
    struct default_serializable_tag {};
    struct custom_serializable_tag {};

    template <typename E>
    struct element
    {
        typedef default_serializable_tag serialization;
    };
}

namespace sequia
{
    template <typename T>
    class stream
    {
        public:
            stream (T *buf, size_t len) 
                : head_(buf), tail_(buf), begin_(buf), end_(buf+len) {}

            template <typename E>
            stream &operator<< (E const &e)
            {
                typename traits::element<E>::serialization tag;

                bool success = put (e, tag);

                return *this;
            }

            template <typename E>
            stream &operator>> (E &e)
            {
                typename traits::element<E>::serialization tag;

                bool success = get (e, tag);

                return *this;
            }

        protected:
            template <typename E>
            inline bool put (E const &e, traits::default_serializable_tag)
            {
                E *ptr = reinterpret_cast<E *> (tail_);
                E *begin = reinterpret_cast<E *> (begin_);
                E *end = reinterpret_cast<E *> (end_);

                *ptr = e, ++ptr;
                ptr = (ptr < end)? ptr : ptr - end + begin;

                tail_ = reinterpret_cast<T *> (ptr);
                assert(tail_ <= end_);

                return true;
            }

            template <typename E>
            inline bool get (E &e, traits::default_serializable_tag)
            {
                E *ptr = reinterpret_cast<E *> (head_);
                E *begin = reinterpret_cast<E *> (begin_);
                E *end = reinterpret_cast<E *> (end_);

                e = *ptr, ++ptr;
                ptr = (ptr < end)? ptr : ptr - end + begin;

                head_ = reinterpret_cast<T *> (ptr);
                assert(head_ <= end_);

                return true;
            }

            template <typename U>
            inline bool put (buffer<U> const &b, traits::default_serializable_tag)
            {
                U *ptr = reinterpret_cast<U *> (head_);
                U *begin = reinterpret_cast<U *> (begin_);
                U *end = reinterpret_cast<U *> (end_);

                *ptr = b.size; ++ptr;
                memcpy(ptr, b.data, b.size * sizeof(U)), ptr += b.size;
                ptr = (ptr < end)? ptr : ptr - end + begin;

                head_ = reinterpret_cast<T *> (ptr);
                assert(head_ <= end_);

                return true;
            }

            template <typename U>
            inline bool get (buffer<U> &b, traits::default_serializable_tag)
            {
                U *ptr = reinterpret_cast<U *> (tail_);
                U *begin = reinterpret_cast<U *> (begin_);
                U *end = reinterpret_cast<U *> (end_);

                b.size = *ptr, ++ptr;
                memcpy(b.data, ptr, b.size * sizeof(U)), ptr += b.size;
                ptr = (ptr < end)? ptr : ptr - end + begin;

                tail_ = reinterpret_cast<T *> (ptr);
                assert(tail_ <= end_);

                return true;
            }

            template <typename E>
            inline bool put (E const &e, traits::custom_serializable_tag)
            {
                return e.serialize(this);
            }

            template <typename E>
            inline bool get (E &e, traits::custom_serializable_tag)
            {
                return e.deserialize(this);
            }

        private:
            T *head_;
            T *tail_;
            T *begin_;
            T *end_;
    };
}

#endif
