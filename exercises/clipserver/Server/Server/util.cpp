
#include "util.hpp"

namespace util
{
    uint64_t generate_rand64 ()
    {
        std::srand (static_cast<unsigned> (std::time (0)));
        // std::rand portably guarantees only 15 bits of randomness (see RAND_MAX)
        uint64_t A = 0xFFFF & std::rand ();
        uint64_t B = 0xFFFF & std::rand ();
        uint64_t C = 0xFFFF & std::rand ();
        uint64_t D = 0xFFFF & std::rand ();
        return (A << (3 * 16)) | (B << (2 * 16)) | (C << (1 * 16)) | (D << (0 * 16));
    }

    uint64_t generate_guid64 (std::string const &bytes)
    {
        time_t now = std::time (0);
        std::srand ((uint32_t) now);

        uint64_t seed = generate_rand64 ();
        uint64_t guid = (now * seed) ^ 0xC96C5795D7870F42;

        std::string::const_iterator byte = bytes.begin ();
        std::string::const_iterator end = bytes.end ();
        for (; byte != end; ++byte)
        {
            guid =
                ((guid & 0xFFFF000000000000) >> (2 * 16)) |
                ((guid & 0x0000FFFF00000000) << (1 * 16)) |
                ((guid & 0x00000000FFFF0000) >> (1 * 16)) |
                ((guid & 0x000000000000FFFF) << (2 * 16));

            guid ^= (*byte * seed) ^ 0x82F63B78EB31D82E;
        }

        return guid;
    }

    uint64_t generate_rand63 ()
    {
        return generate_rand64() & 0x7FFFFFFFFFFFFFFF;
    }

    uint64_t generate_guid63 (std::string const &bytes)
    {
        return generate_guid64 (bytes) & 0x7FFFFFFFFFFFFFFF;
    }
    
    uint64_t generate_hash64 (std::string const &phrase)
    {
        assert (false && "not implemented");
        return 0;
    }
    
    uint64_t generate_hash63 (std::string const &phrase)
    {
        assert (false && "not implemented");
        return 0;
    }
}
