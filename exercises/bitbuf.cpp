
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <array>

using namespace std;

template <typename T>
struct bit_traits;

template <>
struct bit_traits<int8_t>
{
    constexpr static uint8_t zeros = 0;
    constexpr static uint8_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint8_t) * 8;
    constexpr static size_t size_log2 = 3;
};

template <>
struct bit_traits<uint8_t>
{
    constexpr static uint8_t zeros = 0;
    constexpr static uint8_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint8_t) * 8;
    constexpr static size_t size_log2 = 3;
};

template <>
struct bit_traits<int16_t>
{
    constexpr static uint16_t zeros = 0;
    constexpr static uint16_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint16_t) * 8;
    constexpr static size_t size_log2 = 4;
};

template <>
struct bit_traits<uint16_t>
{
    constexpr static uint16_t zeros = 0;
    constexpr static uint16_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint16_t) * 8;
    constexpr static size_t size_log2 = 4;
};

template <>
struct bit_traits<int32_t>
{
    constexpr static uint32_t zeros = 0;
    constexpr static uint32_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint32_t) * 8;
    constexpr static size_t size_log2 = 5;
};

template <>
struct bit_traits<uint32_t>
{
    constexpr static uint32_t zeros = 0;
    constexpr static uint32_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint32_t) * 8;
    constexpr static size_t size_log2 = 5;
};

template <>
struct bit_traits<int64_t>
{
    constexpr static uint64_t zeros = 0;
    constexpr static uint64_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint64_t) * 8;
    constexpr static size_t size_log2 = 6;
};

template <>
struct bit_traits<uint64_t>
{
    constexpr static uint64_t zeros = 0;
    constexpr static uint64_t ones = ~zeros;
    constexpr static size_t size = sizeof (uint64_t) * 8;
    constexpr static size_t size_log2 = 6;
};

struct bitbuffer
{
    uint8_t *base = nullptr;
    uint_fast16_t limit = 0;
    uint_fast8_t offset = 0;

    bitbuffer () = default;

    bitbuffer (uint8_t *data, uint_fast16_t size, uint_fast8_t begin = 0) : 
        base {data}, limit {size}, offset {begin} {} // byte align size

    bitbuffer (uint8_t *begin, uint8_t *end) : 
        base {begin}, limit (((uintptr_t) end - (uintptr_t) begin) * 8), offset {0} {}
};

size_t size (bitbuffer const &buf)
{
    return buf.limit - buf.offset;
}

template <typename Type>
void store (bitbuffer &buf, Type value)
{
    // assert (size (buf) >= bit_traits<Type>::size)
    Type *ptr = (Type *) buf.base + (buf.offset >> bit_traits<Type>::size_log2);
    Type off = buf.offset & bit_traits<Type>::size-1;

    if (off)
    {
        Type dstmask = bit_traits<Type>::ones << off;
        Type srcmask = bit_traits<Type>::ones >> off;
        Type frag1 = ((value & srcmask) << off) | (*ptr & ~dstmask);
        Type frag2 = ((value & ~srcmask) >> off) | (*ptr & dstmask);

        *ptr++ = frag1;
        *ptr = frag2;
    }
    else
        *ptr = value;

    buf.offset += bit_traits<Type>::size;
}

template <>
void store (bitbuffer &buf, bool value)
{
    uint8_t *ptr = buf.base + (buf.offset >> 3);
    uint8_t mask = 1 << (buf.offset & 7);

    *ptr = (value * mask) | (*ptr & ~mask);
    buf.offset += 1;
}


template <typename Type>
void load (bitbuffer &buf, Type &value)
{
    // assert (size (buf) >= bit_traits<Type>::size)
    Type *ptr = (Type *) buf.base + (buf.offset >> bit_traits<Type>::size_log2);
    Type off = buf.offset & bit_traits<Type>::size-1;

    if (off)
    {
        Type mask = bit_traits<Type>::ones << off;
        Type frag1 = *(ptr+0);
        Type frag2 = *(ptr+1);

        value = ((frag1 & mask) >> off) | ((frag2 & ~mask) << (bit_traits<Type>::size - off));
    }
    else
        value = *ptr;

    buf.offset += bit_traits<Type>::size;
}

template <>
void load (bitbuffer &buf, bool &value)
{
    uint8_t *ptr = buf.base + (buf.offset >> 3);
    uint8_t mask = 1 << (buf.offset & 7);

    value = mask & *ptr;
    buf.offset += 1;
}


int main (int argc, char **argv)
{
    int const N = 16;
    uint8_t mem [N];
    bitbuffer buf (mem, mem + N);

    for (uint8_t &byte : mem)
        byte = 0;

    store (buf, (bool) 1);
    store (buf, (uint32_t) 0x10000001);
    store (buf, (uint8_t) 5);

    cout << std::hex;
    cout << "---------------" << endl;
    
    buf.offset = 0;
    bool value1 = false;
    uint8_t value8 = 0;
    uint32_t value32 = 0;

    load (buf, value1); cout << (int) value1 << endl;
    load (buf, value32); cout << (int) value32 << endl;
    load (buf, value8); cout << (int) value8 << endl;

    cout << "---------------" << endl;

    for (uint8_t byte : mem)
        cout << (int) byte << endl;

    return 0;
}
