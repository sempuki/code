// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

template <typename Type>
class Stack {
 public:
  template <typename InputType>
  void push(InputType value) {
    data_.push_back(std::forward<InputType>(value));
  }

  Type pop() {
    Type result = std::move(data_.back());
    data_.pop_back();
    return result;
  }

  bool empty() const {
    return data_.empty();
  }

 private:
  std::vector<Type> data_;
};

void process(std::string &&line) {
  std::stringstream buffer{std::move(line)};
  Stack<int> stack;

  int value = 0;
  while (buffer >> value) {
    stack.push(value);
  }

  bool toggle = false;
  while (!stack.empty()) {
    int value = stack.pop();
    if (toggle ^= true) {
      std::cout << value << ' ';
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
