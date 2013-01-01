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
                    memory::buffer <char const> buf {str, strlen (str)};
                    auto aligned = memory::align <uint32_t const> (buf);
                    
                    uint32_t hash = core::crc32c;

                    // Include any bytes missed due to alignment overhead
                    auto pre = *reinterpret_cast <uint32_t const *> (buf.begin());
                    auto pre_size= memory::aligned_overhead <uint32_t> (buf.begin()) * 8;
                    hash ^= pre >> pre_size;

                    // Hash aligned blocks of 4-byte words
                    for (auto block : aligned)
                    {
                        hash = (hash << 16) | 
                            ((hash & 0xF0000000) >> 24) |
                            ((hash & 0x0F000000) >> 12) |
                            ((hash & 0x00F00000) >> 12) |
                            ((hash & 0x000F0000) >> 16);

                        hash ^= core::crc32c;
                    }

                    // Include any bytes missed due to alignment offset
                    auto post = *reinterpret_cast <uint32_t const *> (buf.end() - 4);
                    auto post_size = memory::aligned_offset <uint32_t> (buf.end()) * 8;
                    hash ^= post >> pre_size;

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
#ifdef DEBUG
            out << n.string();
#else
            out << static_cast <uint32_t> (n);
#endif
            return out;
        } 
    }
}

#endif
