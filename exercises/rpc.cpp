#include <iostream>

using namespace std;

uint32_t htonl (uint32_t value) { return value; }
uint32_t ntohl (uint32_t value) { return value; }

namespace sequia { namespace data { namespace map { 

    namespace local 
    {
        uint8_t *operator<< (uint8_t *buffer, int32_t value)
        {
            *reinterpret_cast <int32_t *> (buffer) = value;
            return buffer + sizeof (int32_t);
        }

        uint8_t *operator>> (uint8_t *buffer, int32_t &value)
        {
            value = *reinterpret_cast <int32_t *> (buffer);
            return buffer + sizeof (int32_t);
        }

        uint8_t *operator<< (uint8_t *buffer, uint32_t value)
        {
            *reinterpret_cast <uint32_t *> (buffer) = value;
            return buffer + sizeof (uint32_t);
        }

        uint8_t *operator>> (uint8_t *buffer, uint32_t &value)
        {
            value = *reinterpret_cast <uint32_t *> (buffer);
            return buffer + sizeof (uint32_t);
        }
    }

    namespace network 
    {
        uint8_t *operator<< (uint8_t *buffer, int32_t value)
        {
            *reinterpret_cast <int32_t *> (buffer) = htonl (value);
            return buffer + sizeof (int32_t);
        }

        uint8_t *operator>> (uint8_t *buffer, int32_t &value)
        {
            value = ntonl (*reinterpret_cast <int32_t *> (buffer));
            return buffer + sizeof (int32_t);
        }

        uint8_t *operator<< (uint8_t *buffer, uint32_t value)
        {
            *reinterpret_cast <uint32_t *> (buffer) = htonl (value);
            return buffer + sizeof (uint32_t);
        }

        uint8_t *operator>> (uint8_t *buffer, uint32_t &value)
        {
            value = ntohl (*reinterpret_cast <uint32_t *> (buffer));
            return buffer + sizeof (uint32_t);
        }
    }

} } }

namespace sequia { namespace core {

    template <typename E>
    class stream
    {
        public:
            stream (E const *buf, int size)
                : mem_ {buf}, capacity_ {size}, 
                  size_ {0}, read_ {0}, write_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                mem_[write_] << item;
                ++write_ %= capacity_;
                ++size_;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                mem_[read_] >> item;
                ++read_ %= capacity_;
                --size_;
            }

            bool full () const { return size_ == capacity_; }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            E *mem_;
            int capacity_, size_, read_, write_;
    };

    template <>
    class stream <uint8_t>
    {
        public:
            stream (uint8_t const *buf, int size)
                : mem_ {buf}, read_ {buf}, write_ {buf}, 
                  capacity_ {size}, size_ {0}, 
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
            uint8_t *mem_, *read_, *write_;
            int capacity_, size_;
    };

    template <>
    class stream <bool>
    {
        // TODO
    };

} }

int main (int argc, char **argv)
{
    //sequia::core::stream<uint8_t> test;

    return 0;
}
