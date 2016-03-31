// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <array>
#include <limits>


void process(std::string &&line) {
  std::stringstream buffer{std::move(line)};

  std::array<int, 20> scores;
  scores.fill(std::numeric_limits<int>::min());

  std::string row;
  while (std::getline(buffer, row, '|')) {
    std::stringstream score{std::move(row)};

    for (int value, i = 0; score; ++i) {
      value = std::numeric_limits<int>::min();
      score >> value;
      scores[i] = std::max(scores[i], value);
    }
  }

  for (auto high : scores) {
    if (high > std::numeric_limits<int>::min()) {
      std::cout << high << ' ';
    }
  }
  std::cout << std::endl;
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
    while (std::getline(file, line)) {
      process(std::move(line));
    }

    return 0;
}
