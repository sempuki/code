#include <iostream>
#include <typeinfo>
#include <cxxabi.h>

using std::cout;
using std::endl;

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
            Type *items = nullptr;
            void const *address;
        };

        size_t bytes = 0;

        buffer () {}

        buffer (Type *data, size_t num_items) : 
            items {data}, bytes {num_items * sizeof (Type)} {}

        buffer (void const *data, size_t num_bytes) :
            address {data}, bytes {num_bytes} {}

        template <typename U> 
        buffer (buffer<U> const &copy) :
            buffer {copy.address, copy.bytes} {}

        //operator bool () const { return address != nullptr && bytes != 0; }
        size_t size () const { return bytes / sizeof (Type); }
    };

    struct bufdiff
    {
        ptrdiff_t addr;
        size_t size;
    };

    template <typename T>
    size_t count (buffer<T> const &buf) { return buf.bytes / sizeof(T); }

    template <typename U, typename T>
    size_t count (buffer<T> const &buf) { return buf.bytes / sizeof(U); }

    template <typename T>
    size_t count_bytes (buffer<T> const &buf) { return buf.bytes; }

    template <typename Type>
    Type *begin (buffer<Type> &buf) { return buf.items; }

    template <typename Type>
    Type const *begin (buffer<Type> const &buf) { return buf.items; }

    template <typename Type>
    Type *end (buffer<Type> &buf) { return begin (buf) + count (buf); }

    template <typename Type>
    Type const *end (buffer<Type> const &buf) { return begin (buf) + count (buf); }

    template <typename Type>
    buffer<Type> shrink (buffer<Type> const &buf, int amount)
    {
        return buffer<Type> {begin (buf), count (buf) - amount};
    }

    template <typename Type>
    buffer<Type> grow (buffer<Type> const &buf, int amount)
    {
        return buffer<Type> {begin (buf), count (buf) + amount};
    }

    template <typename Type>
    buffer<Type> offset (buffer<Type> const &buf, int amount)
    {
        return buffer<Type> {begin (buf) + amount, count (buf)};
    }

    template <typename Type>
    buffer<Type> advance (buffer<Type> const &buf, int amount = 1)
    {
        return buffer<Type> {begin (buf) + amount, count (buf) - amount};
    }

    template <typename Type>
    buffer<Type> retreat (buffer<Type> const &buf, int amount = 1)
    {
        return buffer<Type> {begin (buf) - amount, count (buf) + amount};
    }

    template <typename Type>
    bufdiff difference (buffer<Type> const &a, buffer<Type> const &b)
    {
        return {std::abs (begin (a) - begin (b)), std::abs (count (a) - count (b))};
    }
}

namespace data { namespace map { 

    namespace native 
    {
        template <typename Type>
        memory::buffer<uint8_t> &operator<< (memory::buffer<uint8_t> &buf, Type value)
        {
            memory::buffer<Type> typed {buf};

            cout << "writing: " << value << " to " << (uintptr_t) buf.items << endl;

            //ASSERTF (typed.size(), "no room to write type size of %d", sizeof (Type));
            *typed.items = value;
            buf = memory::advance (typed);

            return buf;
        }

        template <typename Type>
        memory::buffer<uint8_t> &operator>> (memory::buffer<uint8_t> &buf, Type &value)
        {
             memory::buffer<Type> typed {buf};

            //ASSERTF (typed.size(), "no room to read type size of %d", sizeof (Type));
            value = *typed.items;
            buf = memory::retreat (typed);

            return buf;
        }
    }

    namespace network 
    {
        template <typename Type>
        memory::buffer<uint8_t> &operator<< (memory::buffer<uint8_t> &buf, Type value)
        {
            memory::buffer<Type> typed {buf};

            //ASSERTF (typed.size(), "no room to write type size of %d", sizeof (Type));
            *typed.items = make_network_byte_order (value);
            buf = memory::advance (typed);

            return buf;
        }

        template <typename Type>
        memory::buffer<uint8_t> &operator>> (memory::buffer<uint8_t> &buf, Type &value)
        {
            memory::buffer<Type> typed {buf};

            //ASSERTF (typed.size(), "no room to read type size of %d", sizeof (Type));
            value = make_host_byte_order (*typed.items);
            buf = memory::retreat (typed);

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
                ++write_ %= count (buf_);
                ++size_;
                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                buf_.items[read_] >> item;
                ++read_ %= count (buf_);
                --size_;
                return *this;
            }

            bool full () const { return size_ == count (buf_); }
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
                : buf_ {buf}, read_ {0}, write_ {0}, size_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                auto curr = (memory::advance (buf_, write_) << item);
                auto size = (memory::difference (buf_, curr).addr);
                write_ += size;
                write_ %= count (buf_);
                size_ += size;;
                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                //offset (buf_, read_) >> item;
                //++read_ %= count (buf_);
                //--size_;
                return *this;
            }

            bool full () const { return size_ == count (buf_); }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            memory::buffer<uint8_t> buf_;
            int size_, read_, write_;
    };

    template <>
    class stream <bool>
    {
        // TODO
    };

}

int main (int argc, char **argv)
{
    using namespace data::map::native;

    size_t size = 16;
    uint8_t memory[size];

    std::fill_n (memory, size, 0);
    core::stream<uint8_t> stream {{memory, size}};

    stream << (int32_t) 0xABCDDCBA;
    stream << (int32_t) 0xFF00FF00;
    stream << (int32_t) 0x00FF00FF;
    stream << (int32_t) 0xAABBAABB;

    std::cout << std::hex;
    for (int i : memory)
        std::cout << i << std::endl;

    return 0;
}
