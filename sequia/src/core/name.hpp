#ifndef _NAME_HPP_
#define _NAME_HPP_

namespace sequia { namespace core {

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

                auto length = strlen (str);
                memory::buffer <uint32_t const> buf {str, length};
                memory::buffer <uint8_t const> post {buf.end(), length - buf.nbytes()};

                hash = block_hash_32 (buf, hash, core::crc32c);
                hash = block_hash_32 (post, hash, core::crc32c);

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

} }

#endif
