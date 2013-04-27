#ifndef _STREAM_HPP_
#define _STREAM_HPP_

namespace sequia { namespace core {

    template <typename E>
    class stream
    {
        public:
            explicit stream (memory::buffer<E> const buf) 
                : size_ {0}, read_ {0}, write_ {0}, mem_ {buf}
            {
                WATCHF (item_count (mem_) == item_count (buf), "using buffer size %d (of possible %zd)", 
                        item_count (mem_), item_count (buf));
            }

            template <typename T>
            stream &operator<< (T const &item)
            {
                ASSERTF (!full(), "writing to a full stream");

                mem_.items[write_] << item;
                ++write_ &= (item_count (mem_) - 1);
                ++size_;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                ASSERTF (!empty(), "reading from an empty stream");

                mem_.items[read_] >> item;
                ++read_ &= (item_count (mem_) - 1);
                --size_;
            }

            bool full () const { return size_ == item_count (mem_); }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            int size_, read_, write_;
            memory::pow2_size_buffer<E> mem_;
    };

} }

#endif
