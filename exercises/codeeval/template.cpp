// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <fstream>
#include <iostream>
#include <sstream>

void process(std::string &&line) {
    std::stringstream stream{std::move(line)};

    // process here
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
