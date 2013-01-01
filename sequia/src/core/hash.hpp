#ifndef _HASH_HPP_
#define _HASH_HPP_

namespace sequia
{
    namespace core
    {
        template <typename T>
        uint32_t block_hash_32 (memory::buffer<T> const &buf, uint32_t hash, uint32_t magic)
        {
            for (auto block : buf)
            {
                hash = (hash << 16) | 
                    ((hash & 0xF0000000) >> 24) |
                    ((hash & 0x0F000000) >> 12) |
                    ((hash & 0x00F00000) >> 12) |
                    ((hash & 0x000F0000) >> 16);

                hash ^= block;
                hash ^= magic;
            }

            return hash;
        }
    }
}

#endif
