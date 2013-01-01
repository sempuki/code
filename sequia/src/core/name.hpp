#ifndef _NAME_HPP_
#define _NAME_HPP_

namespace sequia
{
    namespace core
    {
        class name
        {
            public:
                name (char const *str) : 
#ifdef DEBUG
                    name_ (str),
#endif
                    hash_ (compute_hash (str)) {}

            public:
                operator uint32_t () const { return hash_; }
#ifdef DEBUG
                char const *string () const { return name_; }
#endif
            public:
               bool operator== (name const &r) { return hash_ == r.hash_; }
               bool operator< (name const &r) { return hash_ < r.hash_; }

            private:
                uint32_t compute_hash (char const *str)
                {
                    uint32_t hash = core::crc32c;

                    // Compute initial buffers
                    memory::buffer <char const> buf {str, strlen (str)};
                    auto aligned = memory::aligned_buffer <uint32_t const> (buf);
                    auto overhead = memory::aligned_overhead <uint32_t const> (buf);

                    std::cout << "buf address: " << buf.address << std::endl;
                    std::cout << "buf size: " << buf.size << std::endl;
                    std::cout << "aligned address: " << aligned.address << std::endl;
                    std::cout << "aligned size: " << aligned.size << std::endl;
                    std::cout << "pointer overhead: " << overhead.first << std::endl;
                    std::cout << "size overhead: " << overhead.second << std::endl;
                    
                    // Include any unaligned bytes at the beginning
                    auto ptr = reinterpret_cast <void const *> (buf.begin());
                    auto size = aligned.size? overhead.first : buf.size;

                    memory::buffer <uint8_t const> prelude {ptr, size};
                    hash = block_hash_32 (prelude, hash, core::crc32c);

                    // Hash aligned blocks of 4-byte words
                    hash = block_hash_32 (aligned, hash, core::crc32c);

                    // Include any unaligned bytes at the end
                    ptr = reinterpret_cast <void const *> (aligned.end());
                    size = aligned.size? overhead.second : 0;

                    memory::buffer <uint8_t const> postlude {ptr, size};
                    hash = block_hash_32 (postlude, hash, core::crc32c);

                    return hash;
                }

            private:
#ifdef DEBUG
                char const *name_;
#endif
                uint32_t    hash_;
        };

        std::ostream &operator<< (std::ostream &out, name const &n)
        {
            out << static_cast <uint32_t> (n);
#ifdef DEBUG
            out << " [" << n.string() << "]";
#endif
            return out;
        } 
    }
}

#endif
