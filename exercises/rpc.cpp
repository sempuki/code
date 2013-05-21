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
            void const *pointer;
            uintptr_t   address;
        };

        size_t bytes = 0;

        buffer () {}

        buffer (Type *data, size_t num_items) : 
            items {data}, bytes {num_items * sizeof (Type)} {}

        buffer (Type *begin, Type *end) : 
            items {begin}, bytes {(end - begin) * sizeof (Type)} {}

        buffer (void const *data, size_t num_bytes) :
            pointer {data}, bytes {num_bytes} {}

        buffer (void const *begin, void const *end) :
            pointer {begin}, bytes {(uintptr_t) end - (uintptr_t) begin} {}

        template <typename U> 
        buffer (buffer<U> const &copy) :
            buffer {copy.pointer, copy.bytes} {}

        operator bool () const { return pointer != nullptr; }
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
    buffer<Type> grow (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf), size (buf) + amount};
    }

    template <typename Type>
    buffer<Type> shrink (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf), size (buf) - amount};
    }

    template <typename Type>
    buffer<Type> advance (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf) + amount, size (buf)};
    }

    template <typename Type>
    buffer<Type> retreat (buffer<Type> &buf, size_t amount)
    {
        return {begin (buf) - amount, size (buf)};
    }

    template <typename Type>
    buffer<Type> offset (buffer<Type> &buf, int amount)
    {
        return {begin (buf) + amount, size (buf) - amount};
    }

    template <typename T, typename U>
    bool preceeds (buffer<T> const &a, buffer<U> const &b)
    {
        return a.address < b.address;
    }

    template <typename T, typename U>
    bool smaller (buffer<T> const &a, buffer<U> const &b)
    {
        return a.bytes < b.bytes;
    }
}

namespace data { namespace map {

    namespace native
    {
        // Buffer .............................................................

        template <typename T>
        size_t commit_size (memory::buffer<T> const &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::buffer<T> const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<T> &operator<< (memory::buffer<T> &buf, T value)
        {
            *begin (buf) = value; 
            return buf = offset (buf, 1);
        }

        template <typename T>
        bool can_extract (memory::buffer<T> const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<T> &operator>> (memory::buffer<T> &buf, T &value)
        {
            value = *begin (buf); 
            return buf = offset (buf, 1);
        }
    }

    namespace network
    {
        // Buffer .............................................................

        template <typename T>
        size_t commit_size (memory::buffer<T> &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::buffer<T> &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<T> &operator<< (memory::buffer<T> &buf, T value)
        {
            *begin (buf) = make_network_byte_order (value); 
            return buf = offset (buf, 1);
        }

        template <typename T>
        bool can_extract (memory::buffer<T> &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<T> &operator>> (memory::buffer<T> &buf, T &value)
        {
            value = make_host_byte_order (*begin (buf)); 
            return buf = offset (buf, 1);
        }
    }

} }

namespace core {

    namespace policy { namespace data { namespace mapper {

        struct native
        {
            template <typename Container, typename Type>
            size_t commit_size (Container const &container, Type const &type)
            {
                using ::data::map::native::commit_size;
                return commit_size (container, type);
            }

            template <typename Destination, typename Type>
            bool can_insert (Destination const &destination, Type const &input)
            {
                using ::data::map::native::can_insert;
                return can_insert (destination, input);
            }

            template <typename Destination, typename Type>
            Destination &insert (Destination &destination, Type input)
            {
                using ::data::map::native::operator<<;
                return destination << input;
            }
            
            template <typename Source, typename Type>
            bool can_extract (Source const &source, Type const &output)
            {
                using ::data::map::native::can_extract;
                return can_extract (source, output);
            }

            template <typename Source, typename Type>
            Source &extract (Source &source, Type &output)
            {
                using ::data::map::native::operator>>;
                return source >> output;
            }
        };

        struct network
        {
            template <typename Container, typename Type>
            size_t commit_size (Container const &container, Type const &type)
            {
                using ::data::map::native::commit_size;
                return commit_size (container, type);
            }

            template <typename Destination, typename Type>
            bool can_insert (Destination const &destination, Type const &input)
            {
                using ::data::map::network::can_insert;
                return can_insert (destination, input);
            }

            template <typename Destination, typename Type>
            Destination &insert (Destination &destination, Type input)
            {
                using ::data::map::network::operator<<;
                return destination << input;
            }
            
            template <typename Source, typename Type>
            bool can_extract (Source const &source, Type const &output)
            {
                using ::data::map::network::can_extract;
                return can_extract (source, output);
            }

            template <typename Source, typename Type>
            Source &extract (Source &source, Type &output)
            {
                using ::data::map::network::operator>>;
                return source >> output;
            }
        };

    } } }

    template <typename E, typename IO>
    class stream : private IO
    {
        public:
            stream (memory::buffer<E> const &buf)
                : buf_ {buf}, size_ {0}, rdpos_ {0}, wrpos_ {0} 
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                IO::insert (buf_.items[wrpos_], item);

                ++wrpos_ %= size (buf_);
                ++size_;

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                IO::extract (buf_.items[rdpos_], item);

                ++rdpos_ %= size (buf_);
                --size_;

                return *this;
            }

            bool full () const { return size_ == size (buf_); }
            bool empty () const { return size_ == 0; }

            size_t in_capacity () const { return size_; }
            size_t out_capacity () const { return size (buf_) - size_; } 

        private:
            memory::buffer<E> buf_;
            int size_, rdpos_, wrpos_;
    };

    template <typename IO>
    class stream <uint8_t, IO> : private IO
    {
        public:
            stream (memory::buffer<uint8_t> const &buf)
                : buf_ {buf}, rdpos_ {0}, wrpos_ {0}, size_ {0}
            {}

            template <typename T>
            stream &operator<< (T const &item)
            {
                auto forward = rdpos_ <= wrpos_;
                auto rdbegin = begin (buf_) + rdpos_;
                auto wrbegin = begin (buf_) + wrpos_;
                auto wrend = forward? end (buf_) : rdbegin;
                auto wrbuf = memory::buffer<T> {wrbegin, wrend};

                if (!IO::can_insert (wrbuf, item) && forward) // try wrap-around
                    wrbuf = {begin (buf_), rdbegin};

                if (IO::can_insert (wrbuf, item))
                {
                    IO::insert (wrbuf, item);
                    wrpos_ = wrbuf.address - buf_.address;
                    size_ += IO::commit_size (wrbuf, item);
                }
                // TODO: else set error flags

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                auto forward = rdpos_ <= wrpos_;
                auto wrbegin = begin (buf_) + wrpos_;
                auto rdbegin = begin (buf_) + rdpos_;
                auto rdend = forward? wrbegin : end (buf_);

                auto rdbuf = memory::buffer<T> {rdbegin, rdend};

                if (!IO::can_extract (rdbuf, item) && !forward) // try wrap-around
                    rdbuf = {begin (buf_), wrbegin};

                if (IO::can_extract (rdbuf, item))
                {
                    IO::extract (rdbuf, item);
                    rdpos_ = rdbuf.address - buf_.address;
                    size_ -= IO::commit_size (rdbuf, item);
                }
                // TODO: else set error flags

                return *this;
            }

            bool full () const { return size_ == 0; }
            bool empty () const { return size_ == 0; }

            size_t in_capacity () const { return size_; }
            size_t out_capacity () const { return size (buf_) - size_; } 

        private:
            memory::buffer<uint8_t> buf_;//, rdbuf_, wrbuf_;
            int rdpos_, wrpos_, size_;
    };

    template <typename IO>
    class stream <bool, IO> : private IO
    {
        // TODO
    };

    template <typename E>
    using basicstream = stream <E, policy::data::mapper::native>;
    using bytestream = stream <uint8_t, policy::data::mapper::native>;
    using bitstream = stream <bool, policy::data::mapper::native>;
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

    //for (uint8_t i = 0; i < size; ++i)
    //    stream << i;

    uint8_t i = 0;
    while (!stream.empty ())
        stream >> i, cout << std::hex << (int) i << endl;

    cout << std::hex;
    for (int i : memory)
        cout << i << endl;

    return 0;
}
