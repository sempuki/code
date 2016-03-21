// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>

#include <array>
#include <algorithm>
#include <cstring>
#include <cctype>


int main (int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "input must be a valid filename" << std::endl;
        return -1;
    }

    std::ifstream file{argv[1]};
    if (!file) {
        std::cout << "input must be a file" << std::endl;
        return -2;
    }

    // Batching for speed.

    std::array<std::array<char, 128>, 40> input;
    size_t count = 0;

    for (auto &&line : input) {
      file.getline(line.data(), line.max_size());
      if (!file) break;
      count++;
    }

    for (int n = 0; n < count; ++n) {
      auto begin = std::begin(input[n]);
      auto end = begin + std::strlen(begin);
      auto write = begin;
      auto read = begin;

      while (read < end) {
        if (std::isalpha(*read)) {
          *write++ = std::tolower(*read++);
        } else {
          read = std::find_if(read, end, [](char ch) {
                return std::isalpha(ch);
              });
          *write++ = ' ';
        }
      }

      *write-- = '\0';
      if (write >= begin && *write == ' ') {
        *write = '\0';
      }

      if (*begin == ' ') {
        memmove(begin, begin+1, std::strlen(begin)+1);
      }
    }

    for (int n = 0; n < count; ++n) {
      std::cout << input[n].data() << std::endl;
    }

    return 0;
}
