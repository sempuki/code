// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

struct Hole {
  std::vector<int> p0;
  std::vector<int> p1;
};

struct Brick {
  int id;
  std::vector<int> p0;
  std::vector<int> p1;
};

std::vector<int> parse_vector(std::string &&buffer) {
  std::vector<int> result;

  std::stringstream stream{std::move(buffer)};
  stream.ignore(1, '[');

  while (stream && stream.peek() != std::string::traits_type::eof()) {
    result.emplace_back();
    stream >> result.back();
    stream.ignore(1, ',');
  }

  return result;
}

std::vector<std::vector<int>> parse_points(std::string &&buffer) {
  std::vector<std::vector<int>> result;

  std::stringstream stream{std::move(buffer)};

  while (std::getline(stream, buffer, ' ')) {
    result.emplace_back(parse_vector(std::move(buffer)));
  }

  return result;
}

Hole parse_hole(std::string &&buffer) {
  auto points = parse_points(std::move(buffer));
  return {points[0], points[1]};
}

Brick parse_brick(std::string &&buffer) {
  Brick result;

  std::stringstream stream{std::move(buffer)};
  stream.ignore(1, '(');
  stream >> result.id;
  stream.ignore(1, ' ');
  std::getline(stream, buffer, ')');

  auto points = parse_points(std::move(buffer));
  result.p0 = points[0];
  result.p1 = points[1];

  return result;
}

void process(std::string &&buffer) {
  std::stringstream stream{std::move(buffer)};

  std::getline(stream, buffer, '|');

  Hole hole = parse_hole(std::move(buffer));
  std::vector<Brick> bricks;

  while (std::getline(stream, buffer, ';')) {
    bricks.emplace_back(parse_brick(std::move(buffer)));
  }

  // start here
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

  std::string line;
  while (std::getline(file, line)) {
    process(std::move(line));
  }

  return 0;
}
