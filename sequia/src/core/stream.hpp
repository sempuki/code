#ifndef _STREAM_HPP_
#define _STREAM_HPP_

namespace sequia
{
    namespace core
    {
        template <typename E>
        class stream
        {
            public:
                stream (memory::buffer<E> const buf) 
                    : size_ {0}, read_ {0}, write_ {0}, mem_ {buf}
                {
                    auto usable_size = 1 << bit::log2_floor (mem_.size);

                    WATCHF (usable_size == mem_.size, "stream using buffer size %d (of %d)", 
                            usable_size, mem_.size);

                    mem_.size = usable_size;
                }

                template <typename T>
                stream &operator<< (T const &item)
                {
                    ASSERTF (!full(), "writing to a full stream");

                    mem_.items[write_] << item;
                    ++write_ &= (mem_.size-1);
                    ++size_;
                }

                template <typename T>
                stream &operator>> (T &item)
                {
                    ASSERTF (!empty(), "reading from an empty stream");

                    mem_.items[read_] >> item;
                    ++read_ &= (mem_.size-1);
                    --size_;
                }

                bool full () const { return size_ == mem_.size; }
                bool empty () const { return size_ == 0; }
                size_t size () const { return size_; }

            private:
                size_t size_;
                int read_, write_;
                memory::buffer<E> mem_;
        };
    }
}

#endif
