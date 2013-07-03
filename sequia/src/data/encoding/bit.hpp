#ifndef DATA_ENCODING_BIT_HPP_
#define DATA_ENCODING_BIT_HPP_

namespace sequia { namespace data { namespace encoding { namespace bit {

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

    bool is_constant (type header) 
    { 
        return type_to_encoding (header) < 0x02; 
    }

    bool is_fixed (type header) 
    { 
        return type_to_encoding (header) >= 0x02 && type_to_encoding (header) < 0x0E; 
    }

    bool is_variable (type header) 
    { 
        return type_to_encoding (header) >= 0x0E; 
    }

    bool is_error (type header) 
    { 
        return type_to_encoding (header) >= 0x10; 
    }

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

} } } }

#endif
