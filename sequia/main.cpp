#include <iostream>
#include <algorithm>
#include <type_traits>
#include <string>

#include <cstdint>
#include <cassert> // TODO: assertf

#include <sys/time.h>

using namespace std;

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
                E *end = reinterpret_cast<E *> (end);

                *ptr++ = e; 
                ptr = (ptr < end)? ptr : ptr - end + begin;

                tail_ = reinterpret_cast<T *> (ptr);

                return true;
            }

            template <typename E>
            inline bool get (E &e, traits::default_serializable_tag)
            {
                E *ptr = reinterpret_cast<E *> (head_);
                E *begin = reinterpret_cast<E *> (begin_);
                E *end = reinterpret_cast<E *> (end);

                e = *ptr++;
                ptr = (ptr < end)? ptr : ptr - end + begin;

                head_ = reinterpret_cast<T *> (ptr);

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

namespace App
{
    class Test
    {
        public:
            Test(int a, float b, char c, string d)
                : a_(a), b_(b), c_(c), d_(d) {}

            template <typename T>
            bool serialize (sequia::stream<T> *s) const
            {
                s << a_ << b_ << c_ << d_;
            }

            template <typename T>
            bool deserialize (sequia::stream<T> *s)
            {
                s >> a_ >> b_ >> c_ >> d_;
            }

        private:
            int     a_;
            float   b_;
            char    c_;
            string  d_;
    };
}

namespace traits
{
    template <>
    struct element <App::Test>
    {
        typedef custom_serializable_tag serialization;
    };
}


int main(int argc, char **argv)
{
    size_t N = sizeof(App::Test) * 10;
    uint8_t buf[N];

    sequia::stream<uint8_t> stream (buf, N);
    App::Test test (1, 0.1, 'a', "test");
    
    stream << N;

    return 0; 
}
