// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <deque>

std::pair<std::string, std::string> split(std::string &&line) {
  std::stringstream buffer{std::move(line)};
  std::string sentence, order;
  std::getline(buffer, sentence, ';');
  std::getline(buffer, order, '\n');
  return {sentence, order};
}

void process(std::string &&line) {
  auto pair = split(std::move(line));

  std::deque<std::string> words;
  std::vector<std::string> results;
  std::vector<int> positions;
  {
    std::string word;
    std::stringstream buffer{std::move(pair.first)};
    while (std::getline(buffer, word, ' ')) {
      words.push_back(std::move(word));
    }
  }
  {
    std::string position;
    std::stringstream buffer{std::move(pair.second)};
    while (std::getline(buffer, position, ' ')) {
      positions.push_back(std::stoi(position));
    }
  }

  results.resize(words.size());
  for (auto position : positions) {
    results.at(position-1) = std::move(words.front());
    words.pop_front();
  }

  for (auto const &result : results) {
    std::cout << (!result.empty() ? result : words.front()) << ' ';
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
