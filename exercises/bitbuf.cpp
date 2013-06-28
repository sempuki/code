
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <array>

using namespace std;

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

void store (bitbuffer &buf, bool value)
{
    uint8_t *ptr = buf.base + (buf.offset >> 3);
    uint8_t mask = 1 << (buf.offset & 7);

    *ptr = (value * mask) | (*ptr & ~mask);
    buf.offset += 1;
}

void load (bitbuffer &buf, bool &value)
{
    uint8_t *ptr = buf.base + (buf.offset >> 3);
    uint8_t mask = 1 << (buf.offset & 7);

    value = mask & *ptr;
    buf.offset += 1;
}

void store (bitbuffer &buf, uint8_t value)
{
    // assert (size (buf) >= 8)
    uint8_t ones = ~0;
    uint8_t *ptr = buf.base + (buf.offset >> 3);
    uint8_t off = buf.offset & 7;

    if (off)
    {
        uint8_t dstmask = ones << off;
        uint8_t srcmask = ones >> off;
        uint8_t frag1 = ((value & srcmask) << off) | (*ptr & ~dstmask);
        uint8_t frag2 = ((value & ~srcmask) >> off) | (*ptr & dstmask);

        *ptr++ = frag1;
        *ptr = frag2;
    }
    else
        *ptr = value;

    buf.offset += 8;
}

void load (bitbuffer &buf, uint8_t &value)
{
    // assert (size (buf) >= 8)
    uint8_t ones = ~0;
    uint8_t *ptr = buf.base + (buf.offset >> 3);
    uint8_t off = buf.offset & 7;

    if (off)
    {
        uint8_t mask = ones << off;
        uint8_t frag1 = *(ptr+0);
        uint8_t frag2 = *(ptr+1);

        value = ((frag1 & mask) >> off) | ((frag2 & ~mask) << (8 - off));
    }
    else
        value = *ptr;

    buf.offset += 8;
}

void store (bitbuffer &buf, uint32_t value)
{
    // assert (size (buf) >= 32)
    uint32_t ones = ~0;
    uint32_t *ptr = (uint32_t *) buf.base + (buf.offset >> 5);
    uint32_t off = buf.offset & 31;

    if (off)
    {
        uint32_t dstmask = ones << off;
        uint32_t srcmask = ones >> off;
        uint32_t frag1 = ((value & srcmask) << off) | (*ptr & ~dstmask);
        uint32_t frag2 = ((value & ~srcmask) >> off) | (*ptr & dstmask);

        *ptr++ = frag1;
        *ptr = frag2;
    }
    else
        *ptr = value;

    buf.offset += 32;
}

void load (bitbuffer &buf, uint32_t &value)
{
    // assert (size (buf) >= 32)
    uint32_t ones = ~0;
    uint32_t *ptr = (uint32_t *) buf.base + (buf.offset >> 5);
    uint32_t off = buf.offset & 31;

    if (off)
    {
        uint32_t mask = ones << off;
        uint32_t frag1 = *(ptr+0);
        uint32_t frag2 = *(ptr+1);

        value = ((frag1 & mask) >> off) | ((frag2 & ~mask) << (32 - off));
    }
    else
        value = *ptr;

    buf.offset += 32;
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
