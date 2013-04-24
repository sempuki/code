#include <iostream>

using namespace std;

uint32_t htonl (uint32_t value) { return value; }
uint32_t ntohl (uint32_t value) { return value; }


namespace memory 
{
    template <typename Type>
    struct buffer
    {
        union 
        {
            Type    *mem;
            uint8_t *bytes = nullptr;
        };

        size_t size = 0;

        buffer (uint8_t *data, size_t bytes) :
            bytes {data}, size {bytes / sizeof(Type)} {}

        template <typename U> 
        buffer (buffer<U> const &copy) :
            buffer {copy.bytes, copy.size * sizeof(U)} {}
    };
}

namespace data { namespace map { 

    namespace local 
    {
        memory::buffer<uint8_t> operator<< (memory::buffer<uint8_t> buf, int32_t value)
        {
            *reinterpret_cast <int32_t *> (buf.mem) = value;
            return buf.mem + sizeof (int32_t);
        }

        memory::buf.mem<uint8_t> operator>> (memory::buffer<uint8_t> buf, int32_t &value)
        {
            value = *reinterpret_cast <int32_t *> (buf.mem);
            return buf.mem + sizeof (int32_t);
        }

        memory::buffer<uint8_t> operator<< (memory::buffer<uint8_t> buf, uint32_t value)
        {
            *reinterpret_cast <uint32_t *> (buf.mem) = value;
            return buf.mem + sizeof (uint32_t);
        }

        memory::buffer<uint8_t> operator>> (memory::buffer<uint8_t> buf, uint32_t &value)
        {
            value = *reinterpret_cast <uint32_t *> (buf.mem);
            return buf.mem + sizeof (uint32_t);
        }
    }

    namespace network 
    {
        memory::buffer<uint8_t> operator<< (memory::buffer<uint8_t> buf, int32_t value)
        {
            *reinterpret_cast <int32_t *> (buf.mem) = htonl (value);
            return buf.mem + sizeof (int32_t);
        }

        memory::buffer<uint8_t> operator>> (memory::buffer<uint8_t> buf, int32_t &value)
        {
            value = ntonl (*reinterpret_cast <int32_t *> (buf.mem));
            return buf.mem + sizeof (int32_t);
        }

        memory::buffer<uint8_t> operator<< (memory::buffer<uint8_t> buf, uint32_t value)
        {
            *reinterpret_cast <uint32_t *> (buf.mem) = htonl (value);
            return buf.mem + sizeof (uint32_t);
        }

        memory::buffer<uint8_t> operator>> (memory::buffer<uint8_t> buf, uint32_t &value)
        {
            value = ntohl (*reinterpret_cast <uint32_t *> (buf.mem));
            return buf.mem + sizeof (uint32_t);
        }
    }

} }

namespace core {

    template <typename E>
    class stream
    {
        public:
            stream (memory::buffer<E> const &buf)
                : mem_ {buf} size_ {0}, read_ {0}, write_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                buf_.mem[write_] << item;
                ++write_ %= buf_.size;
                ++size_;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                buf_.mem[read_] >> item;
                ++read_ %= buf_.size;
                --size_;
            }

            bool full () const { return size_ == capacity_; }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            memory::buffer<E> buf_;
            int size_, read_, write_;
    };

    template <>
    class stream <uint8_t>
    {
        public:
            stream (memory::buffer<uint8_t> const &buf)
                : mem_ {buf}, read_ {buf}, write_ {buf}, size_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                auto next = write_ << item;
                auto size = next - write_;
                write_ = next % capacity_;
                size_ += size;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                auto next = read_ >> item;
                auto size = next - read_;
                read_ = next % capacity_;
                size_ -= size;
            }

            bool full () const { return size_ == capacity_; }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            memory::buffer<uint8_t> mem_, read_, write_;
            int size_;
    };

    template <>
    class stream <bool>
    {
        // TODO
    };

}

int main (int argc, char **argv)
{
    //sequia::core::stream<uint8_t> test;

    return 0;
}
