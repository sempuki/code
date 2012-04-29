#ifndef _CORE_HPP_
#define _CORE_HPP_

#include <cstdio>
#include <cstddef>
#include <cassert>

#if defined(__GNUC__)
#define EXPECT_LIKELY(cond)    __builtin_expect((cond), 1)
#define EXPECT_UNLIKELY(cond)  __builtin_expect((cond), 0)
#else
#define EXPECT_LIKELY(cond)    (cond)
#define EXPECT_UNLIKELY(cond)  (cond)
#endif

#if ((defined DEBUG || defined _DEBUG) && !defined DISABLE_ASSERTS)
#define ASSUMEF(cond, ...) \
    if (EXPECT_UNLIKELY(!(cond))) \
    { \
        printf("--- ASSUMPTION FAILED: "); \
        printf(__VA_ARGS__); \
        printf("\n"); \
    }
#define ASSERTF(cond, ...) \
    if (EXPECT_UNLIKELY(!(cond))) \
    { \
        printf("*** ASSERTION FAILED: "); \
        printf(__VA_ARGS__); \
        printf("\n"); \
        assert(cond); \
    }
#else
#define ASSUMEF(cond, ...) ((void) 0)
#define ASSERTF(cond, ...) ((void) 0)
#endif
    
namespace sequia
{
    //=========================================================================

    constexpr uint64_t one = 1;

    constexpr size_t min_num_bytes (uint64_t value)
    {
       return 
           (value < (one << 8))? 1 : 
           (value < (one << 16) || 0 == (one << 16))? 2 : 
           (value < (one << 32) || 0 == (one << 32))? 4 : 8;
    }

    constexpr size_t min_num_bytes (int64_t value)
    {
        return 
            ((value > -(one << 7)) && (value < (one << 7)))? 1 :
            ((value > -(one << 15)) && (value < (one << 15)))? 2 :
            ((value > -(one << 31)) && (value < (one << 31)))? 4 : 8;
    }

    template <size_t N> struct min_word_type {};
    template <> struct min_word_type <1u> { typedef uint8_t result; };
    template <> struct min_word_type <2u> { typedef uint16_t result; };
    template <> struct min_word_type <4u> { typedef uint32_t result; };
    template <> struct min_word_type <8u> { typedef uint64_t result; };
    
    template <size_t N> 
    struct min_word_size 
    { 
        typedef typename min_word_type<min_num_bytes(N)>::result type;
    };

    //=========================================================================
    
    template <typename T>
    struct buffer
    {
        size_t  size;
        T       *mem;

        buffer () : size (0), mem (0) {}
        buffer (size_t s, T *m) : size (s), mem (m) {}
        buffer (size_t s, void *m) : size (s / sizeof(T)), mem (static_cast<T *>(m)) {}
    };

}

#endif
