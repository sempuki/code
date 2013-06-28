#include <iostream>
#include <typeinfo>
#include <cxxabi.h>
#include <cstdlib>

using std::cout;
using std::endl;

namespace memory 
{
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

    // byte buffer ===========================================================
    
    using bytebuffer = buffer<uint8_t>;
    
    // byte buffer ===========================================================

    using bytebuffer = buffer<uint8_t>;
    
    // bit buffer ============================================================
    
    struct bitbuffer
    {
        uint8_t *base = nullptr;
        uint32_t limit = 0;
        uint32_t offset = 0;

        bitbuffer () = default;

        bitbuffer (uint8_t *data, size_t size, size_t begin = 0) : 
            base {data}, limit (size), offset (begin) {}

        bitbuffer (uint8_t *begin, uint8_t *end) : 
            base {begin}, limit (((uintptr_t) end - (uintptr_t) begin) * 8) {}
    };

    size_t size (bitbuffer const &buf)
    {
        return buf.limit - buf.offset;
    }

    uint8_t *begin (bitbuffer &buf) { return buf.base + (buf.offset >> 3); }
    uint8_t const *begin (bitbuffer const &buf) { return buf.base + (buf.offset >> 3); }
    
    uint8_t *end (bitbuffer &buf) { return buf.base + (buf.limit >> 3); }
    uint8_t const *end (bitbuffer const &buf) { return buf.base + (buf.limit >> 3); }

    uint8_t byte_offset (bitbuffer const &buf) { return buf.offset & 7; }

    bitbuffer offset (bitbuffer &buf, int amount) { buf.offset += amount; return buf; }
}

namespace data { namespace endian {

    enum class type { little, big };

    type detect () 
    { 
        union convert
        {
            uint32_t word;
            uint8_t byte [sizeof(word)];
        };
        
        return (convert {.word = 1}.byte[0] = 0)? type::big : type::little;
    }

    constexpr bool is_big = false;      // TODO: use platform defines to set this
    constexpr bool is_little = !is_big;

    struct big {};          // tag type for big endian conversion
    struct little {};       // tag type for little endian conversion
    using network = big;    // network endian set to big endian
    using native = std::conditional<is_big, big, little>::type;

    uint16_t byte_swap16 (uint16_t value) 
    { 
        // TODO: VC: _byteswap_ushort
        // TODO: GCC 4.8: __builtin_bswap16
        return (value << 8) | (value >> 8);
    }

    uint32_t byte_swap32 (uint32_t value)
    {
        // TODO: VC: _byteswap_ulong
        return __builtin_bswap32 (value);
    }

    uint64_t byte_swap64 (uint64_t value)
    {
        // TODO: VC: _byteswap_uint64
        return __builtin_bswap64 (value);
    }

    template <typename T> T byte_swap (T);
    template <> uint8_t byte_swap (uint8_t value) { return value; }
    template <> uint16_t byte_swap (uint16_t value) { return byte_swap16 (value); }
    template <> uint32_t byte_swap (uint32_t value) { return byte_swap32 (value); }
    template <> uint64_t byte_swap (uint64_t value) { return byte_swap64 (value); }
    template <> int8_t byte_swap (int8_t value) { return value; }
    template <> int16_t byte_swap (int16_t value) { return byte_swap16 (value); }
    template <> int32_t byte_swap (int32_t value) { return byte_swap32 (value); }
    template <> int64_t byte_swap (int64_t value) { return byte_swap64 (value); }

    static_assert (sizeof (float) == sizeof (uint32_t), "float is not 4 bytes");
    template <> float byte_swap (float value) 
    { 
        union convert 
        { 
            float real; 
            uint32_t integer; 
        };

        convert cast {.real = value};
        cast.integer = byte_swap32 (cast.integer); 
        return cast.real;
    }

    static_assert (sizeof (double) == sizeof (uint64_t), "double is not 8 bytes");
    template <> double byte_swap (double value) 
    { 
        union convert 
        { 
            double real; 
            uint64_t integer; 
        };

        convert cast {.real = value};
        cast.integer = byte_swap64 (cast.integer); 
        return cast.real; 
    }

    template <typename From, typename To>
    struct map;

    template <>
    struct map<big, big>
    {
        template <typename T>
        static T convert (T value) { return value; }
    };
    
    template <>
    struct map<big, little>
    {
        template <typename T>
        static T convert (T value) { return byte_swap (value); } 
    };

    template <>
    struct map<little, big>
    {
        template <typename T>
        static T convert (T value) { return byte_swap (value); }
    };
    
    template <>
    struct map<little, little>
    {
        template <typename T>
        static T convert (T value) { return value; }
    };

} }

namespace data { namespace encoding { namespace bit {

    enum class type : uint8_t
    {
        zero,       //  0000
        one,        //  0001
        fixed4u,    //  0010
        fixed4s,    //  0011
        fixed8u,    //  0100
        fixed8s,    //  0101
        fixed16u,   //  0110
        fixed16s,   //  0111
        fixed32u,   //  1000
        fixed32s,   //  1001
        fixed64u,   //  1010
        fixed64s,   //  1011
        fixed128,   //  1100
        fixed256,   //  1101
        variable8,  //  1110
        variable12, //  1111
        error       // 10000
    };

    uint8_t type_to_encoding (type header) { return static_cast <uint8_t> (header); }
    type encoding_to_type (uint8_t header) { return static_cast <type> (header); }
    
    namespace impl
    {
        static constexpr type fixed_type_table [] = 
        { 
            type::zero, 
            type::one, 
            type::fixed4u,
            type::fixed8u,
            type::fixed16u,
            type::fixed32u,
            type::fixed64u
        };
    }

    bool is_constant (type header) { return type_to_encoding (header) < 0x02; }
    bool is_fixed (type header) { return type_to_encoding (header) >= 0x02 && type_to_encoding (header) < 0x0E; }
    bool is_variable (type header) { return type_to_encoding (header) >= 0x0E; }
    bool is_error (type header) { return type_to_encoding (header) >= 0x10; }

    bool is_signed (type header) 
    { 
        // assert (is_fixed (header))
        return type_to_encoding (header) & 0x01; 
    }

    int constant (type header) 
    { 
        // assert (is_constant (header))
        return type_to_encoding (header);
    }

    uint16_t fixed_width (type header)
    {
        // assert (is_fixed (header))
        return (2 << (type_to_encoding (header) >> 1));
    }

    uint16_t variable_encoding_width (type header)
    {
        // assert (is_variable (header))
        return (type_to_encoding (header) & 0x01)? 12 : 8;
    }

    type header_type (uint8_t *pointer, size_t bytes)
    {
        // assert (pointer && bytes != 0 && bytes < 0x2000)
        return (bytes < 0x100)? type::variable8 : type::variable12;
    }

    type header_type (uint64_t value)
    {
        int index = 
            ((value >> 0) > 0) +
            ((value >> 1) > 0) +
            ((value >> 4) > 0) +
            ((value >> 8) > 0) +
            ((value >> 16) > 0) +
            ((value >> 32) > 0);

        return impl::fixed_type_table [index];
    }

    type header_type (uint32_t value)
    {
        int index = 
            ((value >> 0) > 0) +
            ((value >> 1) > 0) +
            ((value >> 4) > 0) +
            ((value >> 8) > 0) +
            ((value >> 16) > 0);

        return impl::fixed_type_table [index];
    }

    type header_type (uint16_t value)
    {
        int index = 
            ((value >> 0) > 0) +
            ((value >> 1) > 0) +
            ((value >> 4) > 0) +
            ((value >> 8) > 0);

        return impl::fixed_type_table [index];
    }

    type header_type (uint8_t value)
    {
        int index = 
            ((value >> 0) > 0) +
            ((value >> 1) > 0) +
            ((value >> 4) > 0);

        return impl::fixed_type_table [index];
    }

    type header_type (bool value)
    {
        int index = value? 1 : 0;

        return impl::fixed_type_table [index];
    }

    type sign_encode (type header, bool negative) 
    { 
        return encoding_to_type (type_to_encoding (header) | negative);
    }

    type header_type (int64_t value)
    {
        uint64_t magnitude = std::abs (value);
        uint8_t negative = value < 0;

        return sign_encode (header_type (magnitude), negative);
    }

    type header_type (int32_t value)
    {
        uint32_t magnitude = std::abs (value);
        uint8_t negative = value < 0;

        return sign_encode (header_type (magnitude), negative);
    }

    type header_type (int16_t value)
    {
        uint16_t magnitude = std::abs (value);
        uint8_t negative = value < 0;

        return sign_encode (header_type (magnitude), negative);
    }

    type header_type (int8_t value)
    {
        uint8_t magnitude = std::abs (value);
        uint8_t negative = value < 0;

        return sign_encode (header_type (magnitude), negative);
    }

    struct value
    {
        type header = type::error;
        uint16_t bitwidth = 0;

        union
        {
            bool     word1;
            uint8_t  word8u;
            int8_t   word8s;
            uint16_t word16u;
            int16_t  word16s;
            uint32_t word32u;
            int32_t  word32s;
            uint64_t word64u;
            int64_t  word64s;
            uint32_t word128[4];
            uint64_t word256[4];
            uint8_t bytes [sizeof(word256)];
            uint8_t *variable;
        };

        value (bool value) : 
            header {header_type (value)}, 
            bitwidth {1},
            word1 {value} {}

        value (uint8_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word8u {value} {}

        value (int8_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word8s (value) {}

        value (uint16_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word16u {endian::map<endian::native,endian::little>::convert (value)} {}

        value (int16_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word16s {endian::map<endian::native,endian::little>::convert (value)} {}

        value (uint32_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word32u {endian::map<endian::native,endian::little>::convert (value)} {}

        value (int32_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word32s {endian::map<endian::native,endian::little>::convert (value)} {}

        value (uint64_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word64u {endian::map<endian::native,endian::little>::convert (value)} {}

        value (int64_t value) : 
            header {header_type (value)}, 
            bitwidth {fixed_width (header)},
            word64s {endian::map<endian::native,endian::little>::convert (value)} {}

        value (uint8_t *pointer, uint16_t bytes) :
            header {header_type (pointer, bytes)}, 
            bitwidth {(uint16_t) (bytes * 8)},
            variable {pointer} {}

        void load (bool &value) 
        {
            // assert (is_constant (header) && bitwidth == 1)
            value = word1;
        }

        void load (uint8_t &value)
        {
            // assert (is_fixed (header) && !is_signed (header) && bitwidth <= (sizeof (value) * 8))
            value = word8u;
        }

        void load (int8_t &value)
        {
            // assert (is_fixed (header) && bitwidth <= (sizeof (value) * 8))
            value = word8s;
        }

        void load (uint16_t &value) 
        {
            // assert (is_fixed (header) && !is_signed (header) && bitwidth <= (sizeof (value) * 8))
            value = word16u;
        }

        void load (int16_t &value)
        {
            // assert (is_fixed (header) && bitwidth <= (sizeof (value) * 8))
            value = word16s;
        }

        void load (uint32_t &value)
        {
            // assert (is_fixed (header) && !is_signed (header) && bitwidth <= (sizeof (value) * 8))
            value = word32u;
        }

        void load (int32_t &value)
        {
            // assert (is_fixed (header) && bitwidth <= (sizeof (value) * 8))
            value = word32s;
        }

        void load (uint64_t &value)
        {
            // assert (is_fixed (header) && !is_signed (header) && bitwidth <= (sizeof (value) * 8))
            value = word64u;
        }

        void load (int64_t &value)
        {
            // assert (is_fixed (header) && bitwidth <= (sizeof (value) * 8))
            value = word64s;
        }

        void load (uint8_t *pointer, size_t bytes)
        {
        }

        size_t size () const { return bitwidth; }
        uint8_t *data () { return is_variable (header)? variable : bytes; }
    };

} } }

namespace data { namespace map {

    namespace native
    {
        // buffer .............................................................

        template <typename T>
        size_t commit_size (memory::bytebuffer const &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::bytebuffer const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator<< (memory::bytebuffer &buf, T value)
        {
            memory::buffer<T> typed = buf;
            *begin (typed) = value; 
            return buf = offset (typed, 1);
        }

        template <typename T>
        bool can_extract (memory::bytebuffer const &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator>> (memory::bytebuffer &buf, T &value)
        {
            memory::buffer<T> typed = buf;
            value = *begin (typed); 
            return buf = offset (typed, 1);
        }

        // bitbuffer  .........................................................

        namespace impl
        {
            namespace bit4
            {
                using data::encoding::bit::value;

                size_t store (value const &container, memory::bitbuffer &buf)
                {
                    return 0;
                }

                size_t load (memory::bitbuffer const &buf, value &container)
                {
                    return 0;
                }
            }
        }

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
            data::encoding::bit::value parsed {value};

            auto bits = impl::bit4::store (parsed, buf);

            return buf = memory::offset (buf, bits);
        }
        
        template <typename T>
        bool can_extract (memory::bitbuffer const &buf, T value)
        {
            return size (buf) >= commit_size (buf, value);
        }

        template <typename T>
        memory::bitbuffer &operator>> (memory::bitbuffer &buf, T &value)
        {
            data::encoding::bit::value parsed;

            auto bits = impl::bit4::load (buf, parsed);
            parsed.load (value);

            return buf = memory::offset (buf, bits);
        }
    }

    namespace network
    {
        // buffer .............................................................

        template <typename T>
        size_t commit_size (memory::bytebuffer &buf, T value)
        {
            return sizeof (value);
        }

        template <typename T>
        bool can_insert (memory::bytebuffer &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator<< (memory::bytebuffer &buf, T value)
        {
            memory::buffer<T> typed = buf;
            *begin (typed) = make_network_byte_order (value); 
            return buf = offset (typed, 1);
        }

        template <typename T>
        bool can_extract (memory::bytebuffer &buf, T value)
        {
            return buf.bytes >= commit_size (buf, value);
        }

        template <typename T>
        memory::bytebuffer &operator>> (memory::bytebuffer &buf, T &value)
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
                using ::data::map::network::commit_size;
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

    // streams ----------------------------------------------------------------
    
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
            stream (memory::bytebuffer const &buf)
                : buf_ {buf}
            {}

            explicit operator bool () const { return !error_; }

        public:
            template <typename T>
            stream &operator<< (T const &item)
            {
                memory::bytebuffer wrbuf {begin (buf_) + wrpos_, size (buf_)};
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
                memory::bytebuffer rdbuf {begin (buf_) + rdpos_, wrpos_};
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
            memory::bytebuffer buf_;
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

    // serialization ----------------------------------------------------------
    
    template <typename Stream, typename T>
    bool serialize (Stream &stream, T arg)
    {
        return (bool) (stream << arg);
    }

    template <typename Stream, typename T, typename ...Types>
    bool serialize (Stream &stream, T arg, Types ...args)
    {
        return (stream << arg) && serialize (stream, std::forward<Types> (args)...);
    }

    template <typename Stream, typename T>
    bool deserialize (Stream &stream, T arg)
    {
        return (bool) (stream >> arg);
    }

    template <typename Stream, typename T, typename ...Types>
    bool deserialize (Stream &stream, T arg, Types ...args)
    {
        return (stream >> arg) && deserialize (stream, std::forward<Types> (args)...);
    }

}

namespace io { namespace net {

    struct message
    {
        int id;
        memory::bytebuffer buffer;
    };

    namespace rpc
    {
        template <typename ...Types>
        bool serialize (message &msg, Types ...args)
        {
            core::bytestream stream {msg.buffer};

            return serialize (stream, std::forward<Types> (args)...);
        }
        
        template <typename ...Types>
        bool deserialize (message &msg, Types ...args)
        {
            core::bytestream stream {msg.buffer};

            return deserialize (stream, std::forward<Types> (args)...);
        }
    }

} }

int main (int argc, char **argv)
{
    size_t size = 16;
    uint8_t memory[size];

    {
        std::fill_n (memory, size, 0);
        core::bytestream stream {{memory, size}};

        //stream << (int32_t) 0xABCDDCBA;
        //stream << (int32_t) 0xFF00FF00;
        //stream << (int32_t) 0x00FF00FF;
        //stream << (int32_t) 0xAABBAABB;

        //uint8_t i = 0;
        //while (!stream.empty ())
        //    stream >> i, cout << std::hex << (int) i << endl;

        //stream.reset ();
        //for (uint8_t i = 0; i < 16; ++i)
        //    stream << i;

        io::net::message msg {1, {memory, size}};
        bool ok = io::net::rpc::serialize (msg, 0xABCDDCBA, 0xFF00FF00, 0x00FF00FF, 0xAABBAABB);

        cout << std::hex;
        for (int i : memory)
            cout << i << endl;
    }
    
    std::cout << "-----------------------------------------" << std::endl;

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
