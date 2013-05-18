#include <iostream>
#include <typeinfo>
#include <cxxabi.h>

using std::cout;
using std::endl;

namespace memory 
{
    // TODO: specialize template for ntoh/hton type or passthrough

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

        operator bool () const { return address != nullptr && bytes != 0; }
        size_t size () const { return bytes / sizeof (Type); }
    };

    template <typename T>
    size_t size (buffer<T> const &buf) { return buf.bytes / sizeof(T); }

    template <typename U, typename T>
    size_t size (buffer<T> const &buf) { return buf.bytes / sizeof(U); }

    template <typename Type>
    Type *begin (buffer<Type> &buf) { return buf.items; }

    template <typename Type>
    Type const *begin (buffer<Type> const &buf) { return buf.items; }

    template <typename Type>
    Type *end (buffer<Type> &buf) { return begin (buf) + size (buf); }

    template <typename Type>
    Type const *end (buffer<Type> const &buf) { return begin (buf) + size (buf); }

    template <typename Type>
    buffer<Type> shrink (buffer<Type> &buf, int amount)
    {
        return {begin (buf), size (buf) - amount};
    }

    template <typename Type>
    buffer<Type> grow (buffer<Type> &buf, int amount)
    {
        return {begin (buf), size (buf) + amount};
    }

    template <typename Type>
    buffer<Type> advance (buffer<Type> &buf, int amount)
    {
        return {begin (buf) + amount, size (buf)};
    }

    template <typename Type>
    buffer<Type> retreat (buffer<Type> &buf, int amount)
    {
        return {begin (buf) - amount, size (buf)};
    }

    template <typename Type>
    buffer<Type> offset (buffer<Type> &buf, int amount)
    {
        return {begin (buf) + amount, size (buf) - amount};
    }

    template <typename T, typename U>
    bool is_before (buffer<T> const &a, buffer<U> const &b)
    {
        return a.address < b.address;
    }

    template <typename T, typename U>
    bool is_smaller (buffer<T> const &a, buffer<U> const &b)
    {
        return a.bytes < b.bytes;
    }

    template <typename T, typename U>
    ptrdiff_t distance (buffer<T> const &a, buffer<U> const &b)
    {
        return reinterpret_cast<intptr_t> (b.address) - 
            reinterpret_cast<intptr_t> (a.address);
    }
    
    template <typename T, typename U>
    ssize_t difference (buffer<T> const &a, buffer<U> const &b)
    {
        return b.bytes - a.bytes;
    }
}

namespace data { namespace map {

    namespace basic
    {
        template <typename T>
        memory::buffer<T> &operator<< (memory::buffer<T> &buf, T value)
        {
            if (buf.bytes >= sizeof (T))
            {
                *begin (buf) = value; 
                buf = offset (buf, 1);
            }
            else
                buf = {};

            return buf;
        }

        template <typename T>
        memory::buffer<T> &operator>> (memory::buffer<T> &buf, T &value)
        {
            if (buf.bytes >= sizeof (T))
            {
                value = *begin (buf);
                buf = offset (buf, 1);
            }
            else
                buf = {};

            return buf;
        }
    }

    namespace network
    {
        template <typename T>
        memory::buffer<T> &operator<< (memory::buffer<T> &buf, T value)
        {
            if (buf.bytes >= sizeof (T))
            {
                *begin (buf) = make_network_byte_order (value); 
                buf = offset (buf, 1);
            }
            else
                buf = {};

            return buf;
        }

        template <typename T>
        memory::buffer<T> &operator>> (memory::buffer<T> &buf, T &value)
        {
            if (buf.bytes >= sizeof (T))
            {
                value = make_host_byte_order (*begin (buf));
                buf = offset (buf, 1);
            }
            else
                buf = {};

            return buf;
        }
    }

} }

namespace core {

    namespace policy { namespace data { namespace dispatch {

        struct basic
        {
            template <typename Destination, typename Type>
            Destination &on_insert (Destination &destination, Type input)
            {
                using ::data::map::basic::operator<<;
                return destination << input;
            }
            
            template <typename Source, typename Type>
            Source &on_extract (Source &source, Type &output)
            {
                using ::data::map::basic::operator>>;
                return source >> output;
            }
        };

        struct network
        {
            template <typename Destination, typename Type>
            Destination &on_insert (Destination &destination, Type input)
            {
                using ::data::map::network::operator<<;
                return destination << input;
            }
            
            template <typename Source, typename Type>
            Source &on_extract (Source &source, Type &output)
            {
                using ::data::map::network::operator>>;
                return source >> output;
            }
        };

    } } }

    template <typename E, typename Dispatcher>
    class stream : private Dispatcher
    {
        public:
            stream (memory::buffer<E> const &buf)
                : buf_ {buf}, size_ {0}, read_ {0}, write_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                Dispatcher::on_insert (buf_.items[write_], item);

                ++write_ %= size (buf_);
                ++size_;

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                Dispatcher::on_extract (buf_.items[read_], item);

                ++read_ %= size (buf_);
                --size_;

                return *this;
            }

            bool full () const { return size_ == size (buf_); }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            memory::buffer<E> buf_;
            int size_, read_, write_;
    };

    template <typename Dispatcher>
    class stream <uint8_t, Dispatcher> : private Dispatcher
    {
        public:
            stream (memory::buffer<uint8_t> const &buf)
                : buf_ {buf}, read_ {begin (buf), 0}, write_ {buf}
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                memory::buffer<T> curr {write_};
                memory::buffer<T> next = Dispatcher::on_insert (curr, item);

                if (!next && is_before (read_, write_))
                {
                    auto window_size = (size_t) std::abs (distance (buf_, read_));
                    curr = {buf_.address, window_size};
                    next = Dispatcher::on_insert (curr, item);
                }

                if (next) 
                {
                    size_ += distance (curr, next);
                    write_ = next;
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                memory::buffer<T> curr {read_};
                memory::buffer<T> next = Dispatcher::on_extract (curr, item);

                if (!next && is_before (write_, read_))
                {
                    auto window_size = (size_t) std::abs (distance (buf_, read_));
                    curr = {buf_.address, window_size};
                    next = Dispatcher::on_extract (curr, item);
                }

                if (next) 
                {
                    size_ -= distance (curr, next);
                    read_ = next;
                }

                return *this;
            }

            bool full () const { return size_ == size (buf_); }
            bool empty () const { return size_ == 0; }
            int size () const { return size_; }

        private:
            memory::buffer<uint8_t> buf_, read_, write_;
            int size_;
    };

    template <typename Dispatcher>
    class stream <bool, Dispatcher> : private Dispatcher
    {
        // TODO
    };

    template <typename E>
    using basicstream = stream <E, policy::data::dispatch::basic>;
    using bytestream = stream <uint8_t, policy::data::dispatch::basic>;
    using bitstream = stream <bool, policy::data::dispatch::basic>;
}

int main (int argc, char **argv)
{
    size_t size = 16;
    uint8_t memory[size];

    std::fill_n (memory, size, 0);
    core::bytestream stream {{memory, size}};

    stream << (int32_t) 0xABCDDCBA;
    stream << (int32_t) 0xFF00FF00;
    stream << (int32_t) 0x00FF00FF;
    stream << (int32_t) 0xAABBAABB;

    cout << std::hex;
    for (int i : memory)
        cout << i << endl;

    return 0;
}
