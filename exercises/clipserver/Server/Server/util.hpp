#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include "standard.hpp"

namespace util
{
    uint64_t generate_rand64 ();
    uint64_t generate_guid64 (std::string const &bytes);

    uint64_t generate_rand63 ();
    uint64_t generate_guid63 (std::string const &bytes);

    uint64_t generate_hash64 (std::string const &phrase);
    uint64_t generate_hash63 (std::string const &phrase);

    class spin_mutex
    {
        public:
            void lock () { while (locked_.exchange (true)); }
            void unlock () { locked_ = false; }

        private:
            std::atomic<bool> locked_ {false};
    };
}

#endif
