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

    // buffer =================================================================
    
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

        buffer () = default;

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

        explicit operator bool () const { return pointer != nullptr; }
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
    
    // bit buffer ============================================================
    
    struct bitbuffer
    {
        uint8_t *base = nullptr;
        uint32_t limit = 0;
        uint32_t offset = 0;

        bitbuffer () = default;

        bitbuffer (uint8_t *data, uint32_t size, uint32_t begin = 0) : 
            base {data}, limit {size}, offset {begin} {}

        bitbuffer (uint8_t *begin, uint8_t *end) : 
            base {begin}, limit {((uintptr_t) end - (uintptr_t) begin) * 8} {}
    };

    size_t size (bitbuffer const &buf)
    {
        return buf.limit - buf.offset;
    }

    uint8_t *begin (bitbuffer &buf) { return buf.base + (buf.offset >> 3); }
    uint8_t const *begin (bitbuffer const &buf) { return buf.base + (buf.offset >> 3); }
    
    uint8_t *end (bitbuffer &buf) { return buf.base + (buf.limit >> 3); }
    uint8_t const *end (bitbuffer const &buf) { return buf.base + (buf.limit >> 3); }
}

namespace data { namespace map {

    namespace native
    {
        // buffer .............................................................

        template <typename T>
        size_t commit_size (memory::buffer<uint8_t> const &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::buffer<uint8_t> const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<uint8_t> &operator<< (memory::buffer<uint8_t> &buf, T value)
        {
            memory::buffer<T> typed = buf;
            *begin (typed) = value; 
            return buf = offset (typed, 1);
        }

        template <typename T>
        bool can_extract (memory::buffer<uint8_t> const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<uint8_t> &operator>> (memory::buffer<uint8_t> &buf, T &value)
        {
            memory::buffer<T> typed = buf;
            value = *begin (typed); 
            return buf = offset (typed, 1);
        }

        // bitbuffer  .........................................................

        template <typename T>
        size_t commit_size (memory::bitbuffer const &buf, T value)
        {
            return sizeof (value) * 8;
        }

        template <typename T>
        bool can_insert (memory::bitbuffer const &buf, T value)
        {
            return size (buf) >= commit_size (buf, value);
        }

        template <typename T>
        memory::bitbuffer &operator<< (memory::bitbuffer &buf, T value)
        {
            return buf;
        }

        template <typename T>
        bool can_extract (memory::bitbuffer const &buf, T value)
        {
            return size (buf) >= commit_size (buf, value);
        }

        template <typename T>
        memory::bitbuffer &operator>> (memory::bitbuffer &buf, T &value)
        {
            return buf;
        }
    }

    namespace network
    {
        // buffer .............................................................

        template <typename T>
        size_t commit_size (memory::buffer<uint8_t> &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::buffer<uint8_t> &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<uint8_t> &operator<< (memory::buffer<uint8_t> &buf, T value)
        {
            memory::buffer<T> typed = buf;
            *begin (typed) = make_network_byte_order (value); 
            return buf = offset (typed, 1);
        }

        template <typename T>
        bool can_extract (memory::buffer<uint8_t> &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::buffer<uint8_t> &operator>> (memory::buffer<uint8_t> &buf, T &value)
        {
            memory::buffer<T> typed = buf;
            value = make_host_byte_order (*begin (typed)); 
            return buf = offset (typed, 1);
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
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                error_ = error_ || wrpos_ == size (buf_);

                if (!error_)
                {
                    IO::insert (buf_.items[wrpos_], item);

                    ++wrpos_ %= size (buf_);
                    ++size_;
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                error_ = error_ || rdpos_ == wrpos_;

                if (!error_)
                {
                    IO::extract (buf_.items[rdpos_], item);

                    ++rdpos_ %= size (buf_);
                    --size_;
                }

                return *this;
            }

            bool full () const { return size_ == size (buf_); }
            bool empty () const { return size_ == 0; }

            size_t capacity () const { return size (buf_); }
            size_t occupied () const { return size_; }
            size_t vacant () const { return size (buf_) - size_; } 

            void reset () { error_ = false; size_ = rdpos_ = wrpos_ = 0; }

        private:
            memory::buffer<E> buf_;
            size_t size_ = 0, rdpos_ = 0, wrpos_ = 0;
            bool error_ = false;
    };

    template <typename IO>
    class stream <uint8_t, IO> : private IO
    {
        public:
            stream (memory::buffer<uint8_t> const &buf)
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                memory::buffer<uint8_t> wrbuf {begin (buf_) + wrpos_, size (buf_)};
                error_ = error_ || IO::can_insert (wrbuf, item) == false;

                if (!error_)
                {
                    IO::insert (wrbuf, item);
                    wrpos_ += IO::commit_size (wrbuf, item);
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                memory::buffer<uint8_t> rdbuf {begin (buf_) + rdpos_, wrpos_};
                error_ = error_ || IO::can_extract (rdbuf, item) == false;

                if (!error_)
                {
                    IO::extract (rdbuf, item);
                    rdpos_ += IO::commit_size (rdbuf, item);
                }

                return *this;
            }

            bool full () const { return wrpos_ == size (buf_); }
            bool empty () const { return wrpos_ == rdpos_; }

            size_t capacity () const { return size (buf_); }
            size_t occupied () const { return wrpos_ - rdpos_; }
            size_t vacant () const { return size (buf_) - wrpos_; } 

            void reset () { error_ = false; rdpos_ = wrpos_ = 0; }

        private:
            memory::buffer<uint8_t> buf_;
            size_t rdpos_ = 0, wrpos_ = 0;
            bool error_ = false;
    };

    template <typename IO>
    class stream <bool, IO> : private IO
    {
        public:
            stream (memory::bitbuffer const &buf)
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                memory::bitbuffer wrbuf {buf_.base, buf_.limit, wrpos_};
                error_ = error_ || IO::can_insert (wrbuf, item) == false;

                if (!error_)
                {
                    IO::insert (wrbuf, item);
                    rdpos_ += IO::commit_size (wrbuf, item);
                }

                return *this;
            }

            template <typename T>
            stream &operator>> (T &item)
            {
                memory::bitbuffer rdbuf {buf_.base, wrpos_, rdpos_};
                error_ = error_ || IO::can_extract (rdbuf, item) == false;

                if (!error_)
                {
                    IO::extract (rdbuf, item);
                    rdpos_ += IO::commit_size (rdbuf, item);
                }

                return *this;
            }

            bool full () const { return wrpos_ == size (buf_); }
            bool empty () const { return wrpos_ == rdpos_; }

            size_t capacity () const { return size (buf_); }
            size_t occupied () const { return wrpos_ - rdpos_; }
            size_t vacant () const { return size (buf_) - wrpos_; } 

            void reset () { error_ = false; rdpos_ = wrpos_ = 0; }

        private:
            memory::bitbuffer buf_;
            size_t rdpos_ = 0, wrpos_ = 0;
            bool error_ = false;
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

    {
        std::fill_n (memory, size, 0);
        core::bytestream stream {{memory, size}};

        stream << (int32_t) 0xABCDDCBA;
        stream << (int32_t) 0xFF00FF00;
        stream << (int32_t) 0x00FF00FF;
        stream << (int32_t) 0xAABBAABB;

        uint8_t i = 0;
        while (!stream.empty ())
            stream >> i, cout << std::hex << (int) i << endl;

        stream.reset ();
        for (uint8_t i = 0; i < 16; ++i)
            stream << i;

        cout << std::hex;
        for (int i : memory)
            cout << i << endl;
    }

    {
        std::fill_n (memory, size, 0);
        core::bitstream stream {{memory, size * 8}};

        stream << 'A';
        
        cout << std::hex;
        for (int i : memory)
            cout << i << endl;
    }

    return 0;
}
