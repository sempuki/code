// `env $CXXFLAGS='--std=c++20 -O3' c++ parser.cpp -o parser`
//   Copyright: Ryan McDougall

#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>

struct Context final {
  std::map<std::string, std::vector<std::size_t>> words;
};

void process_line(std::size_t line_position, std::string &&line, Context *context) {
  std::stringstream stream{std::move(line)};
  std::string word;

  while (stream) {
    stream >> std::ws;
    std::size_t word_position = stream.tellg();
    if (stream >> word) {
      context->words[word].push_back(line_position + word_position);
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "input must be a valid filename" << std::endl;
    return -1;
  }

  std::ifstream file{argv[1]};
  if (!file) {
    std::cout << "input must be a file" << std::endl;
    return -2;
  }

  Context context;
  while (file) {
    std::size_t line_position = file.tellg();
    std::string line;

    if (std::getline(file, line)) {
      process_line(line_position, std::move(line), std::addressof(context));
    }
  }

  for (auto &&word_to_positions : context.words) {
    std::cout << word_to_positions.first << ' ';
    for (auto &&position : word_to_positions.second) {
      std::cout << position << ' ';
    }
    std::cout << '\n';
  }

  return 0;
}
