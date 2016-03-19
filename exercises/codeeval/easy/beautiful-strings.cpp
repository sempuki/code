// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>

#include <array>
#include <algorithm>
#include <cctype>
#include <functional>

void process(const std::string &line) {
  std::array<int, 26> values;
  std::fill_n(begin(values), 26, 0);

  for (auto ch : line) {
    if (std::isalpha(ch)) {
        values.at(std::tolower(ch) - 'a')++;
    }
  }

  size_t beauty = 26, total = 0;
  std::sort(begin(values), end(values), std::greater<int>{});
  for (auto value : values) {
    total += value * beauty--;
  }

  std::cout << total << std::endl;
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

    std::string line;
    while (!std::getline(file, line).eof()) {
      process(line);
    }

    return 0;
}
