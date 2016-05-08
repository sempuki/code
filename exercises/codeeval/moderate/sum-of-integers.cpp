// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>

void process(std::string &&line) {
  std::stringstream buffer{std::move(line)};

  int max = 0;
  int run = 0;
  bool scanning = false;

  std::string number;
  while (std::getline(buffer, number, ',')) {
    auto value = std::stoll(number);
    if (value > 0) {
      run += value;
      scanning = true;
    } else {
      if (scanning) {
        scanning = false;
        max = std::max(max, run);
        run = 0;
      } 
    }
  }

  std::cout << max << std::endl;
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
