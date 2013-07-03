#ifndef DATA_ENDIAN_HPP_
#define DATA_ENDIAN_HPP_

namespace sequia { namespace data { namespace endian {

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

} } }

#endif
