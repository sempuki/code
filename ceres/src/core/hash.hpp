#ifndef _HASH_HPP_
#define _HASH_HPP_

namespace ceres { namespace core {

    // Design: simple and fast 32bit value for any buffered input.
    // * Diffusion: xor against constants with uniformly distributed bits
    // * Mixing: shift bits so entropy is shared across the word
    // * Avalanching: multiply so small changes in input cause to big changes in output
    // * Pre/Post combining safeguards against poor choices of seed hash or magic
    //  Not intended to have any security properties. Constants taken from MurmurHash3.
    
    template <typename T>
    uint32_t block_hash_32 (memory::buffer<T> const &buf, uint32_t hash, uint32_t magic)
    {
        // WARN: algorithm is endian sensitive
        // Are unaligned reads faster/safer with memcpy?

        hash ^= (buf.bytes * 0xcc9e2d51) ^ 0x85ebca6b;

        for (auto block : buf)
        {
            hash = ((hash << 16) |
                   ((hash & 0xF0000000) >> 24) |
                   ((hash & 0x0FF00000) >> 12) |
                   ((hash & 0x000F0000) >> 16));

            hash ^= (block * ~magic) ^ ((~block * magic) >> 16);
        }

        hash ^= (hash * 0xc2b2ae35) ^ ((hash * 0x1b873593) >> 16);
        
        return hash;
    }

} }

#endif
