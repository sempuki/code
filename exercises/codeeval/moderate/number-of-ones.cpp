// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <algorithm>

std::array<uint8_t, 64> table;

void process(std::string &&line) {
  size_t count = 0;
  uint64_t pattern = std::stoll(line);
  while (pattern) {
    count += table[pattern & 0x000000000000003F];
    pattern >>= 6;
  }
  std::cout << count << std::endl;
}

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

    std::array<uint8_t, 8> count = {
      0, 1, 1, 2, 1, 2, 2, 3
    };

    for (size_t i = 0; i < 8; ++i) {
      auto countbegin = begin(count);
      auto tablebegin = begin(table) + 8 * i;
      auto tableend = begin(table) + 8 * (i + 1);

      std::fill(tablebegin, tableend, count[i]); 
      std::transform(tablebegin, tableend, countbegin, tablebegin,
          std::plus<uint8_t>());
    }

    std::string line;
    while (std::getline(file, line)) {
      process(std::move(line));
    }

    return 0;
}
