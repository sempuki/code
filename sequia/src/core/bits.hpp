#ifndef _BITS_HPP_
#define _BITS_HPP_

namespace sequia { namespace core { namespace bit {

    //=====================================================================
    // TODO: MSVC intrinsics
    // TODO: split into multiple headers
    // TODO: split into platform namespace

    inline uint32_t first_one (uint32_t x)
    {
        static_assert (sizeof(uint32_t) == sizeof(unsigned int), "type mismatch");
        ASSERTF (x != 0, "result for zero is undefined");

#if defined __GNUC__
        return 31u - __builtin_clz (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    inline uint32_t leading_zeros (uint32_t x)
    {
        static_assert (sizeof(uint32_t) == sizeof(unsigned int), "type mismatch");
        ASSERTF (x != 0, "result for zero is undefined");

#if defined __GNUC__
        return __builtin_clz (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    inline uint32_t trailing_zeros (uint32_t x)
    {
        static_assert (sizeof(uint32_t) == sizeof(unsigned int), "type mismatch");
        ASSERTF (x != 0, "result for zero is undefined");

#if defined __GNUC__
        return __builtin_ctz (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    inline uint32_t count (uint32_t x)
    {
        static_assert (sizeof(uint32_t) == sizeof(unsigned int), "type mismatch");

#if defined __GNUC__
        return __builtin_popcount (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    inline uint32_t parity (uint32_t x)
    {
        static_assert (sizeof(uint32_t) == sizeof(unsigned int), "type mismatch");

#if defined __GNUC__
        return __builtin_parity (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    inline int32_t swap (int32_t x)
    {
#if defined __GNUC__
        return __builtin_bswap32 (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    inline int64_t swap (int64_t x)
    {
#if defined __GNUC__
        return __builtin_bswap64 (x);
#else
        ASSERTF (false, "not implemented");
        return 0;
#endif
    }

    template <typename T>
    inline bool is_power_2 (T value)
    {
        static_assert (std::is_integral<T>::value, "type is not an integer");

        return (value > 0) && ((value & (value - 1)) == 0);
    }

    inline uint32_t log2_floor (uint32_t x)
    {
        return first_one (x);
    }

    inline uint32_t log2_ceil (uint32_t x)
    {
        return first_one (x) + !is_power_2 (x);
    }

} } }

#endif
