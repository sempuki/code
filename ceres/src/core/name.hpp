#ifndef CORE_NAME_HPP_
#define CORE_NAME_HPP_

namespace ceres { namespace core {

    class name
    {
        public:
            name () : hash_ {0} {}

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
                // NOTE: std::hash would require extra copy in std::string

                uint32_t hash = core::crc32c;

                auto bytes = strlen (str);
                memory::buffer <uint32_t const> buf {str, bytes};

                auto remainder = bytes - buf.bytes;
                memory::buffer <uint8_t const> post {end (buf), remainder};

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

} }

#endif
