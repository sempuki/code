#include <iostream>

using namespace std;

namespace memory 
{
    template <typename Type>
    Type make_network_byte_order (Type value) { return value; }

    template <typename Type>
    Type make_host_byte_order (Type value) { return value; }

    template <typename Type>
    struct buffer
    {
        union 
        {
            Type    *items;
            uint8_t *bytes = nullptr;
        };

        size_t size = 0;

        buffer (uint8_t *data, size_t bytes) :
            bytes {data}, size {bytes / sizeof (Type)} {}

        template <typename U> 
        buffer (buffer<U> const &copy) :
            buffer {copy.bytes, copy.size * sizeof (Type)} {}

        template <typename U> 
        buffer &operator= (buffer<U> const &copy)
        {
            *this = buffer {copy};
            return *this;
        }
    };

    template <typename Type>
    size_t byte_count (buffer<Type> const &buf) { return buf.size * sizeof (Type); }

    template <typename Type>
    size_t item_count (buffer<Type> const &buf) { return buf.size; }

    template <typename Type>
    Type *begin (buffer<Type> &buf) { return buf.items; }

    template <typename Type>
    Type const *begin (buffer<Type> const &buf) { return buf.items; }

    template <typename Type>
    Type *end (buffer<Type> &buf) { return buf.items + buf.size; }

    template <typename Type>
    Type const *end (buffer<Type> const &buf) { return buf.items + buf.size; }
}

namespace data { namespace map { 

    namespace native 
    {
        template <typename Type>
        memory::buffer<uint8_t> &operator<< (memory::buffer<uint8_t> &buf, Type value)
        {
            memory::buffer<Type> typed {buf};

            *typed.items = value;
            ++typed.items;
            --typed.size;

            return buf;
        }

        template <typename Type>
        memory::buffer<uint8_t> &operator>> (memory::buffer<uint8_t> &buf, Type &value)
        {
             memory::buffer<Type> typed {buf};

            value = *typed.items;
            --typed.items;
            ++typed.size;

            return buf;
        }
    }

    namespace network 
    {
        template <typename Type>
        memory::buffer<uint8_t> &operator<< (memory::buffer<uint8_t> &buf, Type value)
        {
            memory::buffer<Type> typed {buf};

            *typed.items = make_network_byte_order (value);
            ++typed.items;
            --typed.size;

            return buf;
        }

        template <typename Type>
        memory::buffer<uint8_t> &operator>> (memory::buffer<uint8_t> &buf, Type &value)
        {
             memory::buffer<Type> typed {buf};

            value = make_host_byte_order (*typed.items);
            --typed.items;
            ++typed.size;

            return buf;
        }
    }

} }

namespace core {

    template <typename E>
    class stream
    {
        public:
            stream (memory::buffer<E> const &buf)
                : buf_ {buf}, size_ {0}, read_ {0}, write_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                buf_.items[write_] << item;
                ++write_ %= buf_.size;
                ++size_;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                buf_.items[read_] >> item;
                ++read_ %= buf_.size;
                --size_;
            }

            bool full () const { return size_ == buf_.size; }
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
                : buf_ {buf}, read_ {buf}, write_ {buf}, size_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                auto next = write_ << item;
                auto size = next - write_;
                write_ = next % buf_.size;
                size_ += size;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                auto next = read_ >> item;
                auto size = next - read_;
                read_ = next % buf_.size;
                size_ -= size;
            }

            bool full () const { return size_ == buf_.size; }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            memory::buffer<uint8_t> buf_, read_, write_;
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
    using data::map::native::operator<<;

    uint8_t memory[16];
    memory::buffer<uint8_t> buf {memory, 16};
    buf << (int32_t) 0xABCDDCBA;
    cout << "size: " << buf.size << endl;

    for (int i : buf)
        cout << std::hex << i << endl;

    return 0;
}
