// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <vector>

struct Time {
  int hour = 0;
  int minute = 0;
  int second = 0;
};

std::ostream &operator<<(std::ostream &out, const Time &time) {
  out
    << std::setw(2) << time.hour << ':'
    << std::setw(2) << time.minute << ':'
    << std::setw(2) << time.second;
  return out;
}

std::istream &operator>>(std::istream &in, Time &time) {
  in >> time.hour;
  in.ignore(1, ':');
  in >> time.minute;
  in.ignore(1, ':');
  in >> time.second;
  return in;
}

Time parse(const char *str) {
  std::stringstream buffer{str};
  Time time; buffer >> time;
  return time;
}

void process(const char *line) {
  std::stringstream buffer{line};
  std::vector<Time> times; times.reserve(20);

  char str[10];
  while (buffer.getline(str, 10, ' ')) {
    times.push_back(parse(str));
  }

  if (!buffer.fail()) {
    times.push_back(parse(str));
  }

  std::sort(begin(times), end(times), [](const Time &a, const Time &b) {
      return a.hour > b.hour ||
        (a.hour == b.hour && a.minute > b.minute) ||
        (a.hour == b.hour && a.minute == b.minute && a.second > b.second);
    });

  std::cout << std::setfill('0');
  for (auto time : times) {
    std::cout << time << ' ';
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

    char line[200];
    while (file.getline(line, 200)) {
      process(line);
    }

    return 0;
}
