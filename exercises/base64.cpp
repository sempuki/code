#include <algorithm>
#include <cassert>
#include <cctype>
#include <iterator>
#include <string>

static const char b64_table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64,
    64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64};

std::string base64_encode(const std::string &bindata) {
    std::string result{(((bindata.size() + 2) / 3) * 4)};  // pre-allocate
    std::fill_n(std::rbegin(result), 4, '=');              // pre-pad

    size_t output = 0;
    size_t accumulator = 0;
    int nbits = 0;

    for (auto ch : bindata) {
        accumulator = (accumulator << 8) | (ch & 0xFFu);
        nbits += 8;
        while (nbits >= 6) {
            nbits -= 6;
            result[output++] = b64_table[(accumulator >> nbits) & 0x3Fu];
        }
    }

    if (nbits > 0) {  // Any trailing bits that are missing.
        assert(nbits < 6);
        accumulator <<= 6 - nbits;
        result[output++] = b64_table[accumulator & 0x3Fu];
    }

    assert(output >= (result.size() - 2));
    assert(output <= result.size());

    return result;
}

std::string base64_decode(const std::string &textdata) {
    std::string result;
    size_t accumulator = 0;
    int nbits = 0;

    for (auto ch : textdata) {
        assert((ch < 127) && (ch >= 0) && (reverse_table[ch] < 64));
        accumulator = (accumulator << 6) | reverse_table[ch];
        nbits += 6;
        if (nbits >= 8) {
            nbits -= 8;
            result += static_cast<char>((accumulator >> nbits) & 0xFFu);
        }
    }

    return result;
}
