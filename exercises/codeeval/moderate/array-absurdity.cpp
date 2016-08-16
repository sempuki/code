// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

void process(std::string &&line) {
    std::stringstream stream{std::move(line)};
    std::string buffer;
    std::vector<int> array;

    std::getline(stream, buffer, ';');
    array.reserve(std::stoi(buffer));
    while (std::getline(stream, buffer, ',')) {
        array.push_back(std::stoi(buffer));
    }

    bool found = false;
    size_t index = 0;
    while (index < array.size()) {
        while (index != array[index]) {
            if (array[index] == array[array[index]]) {
                std::cout << array[index] << std::endl;
                return;
            }
            std::swap(array[index], array[array[index]]);
        }
        index++;
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

    std::string line;
    while (std::getline(file, line)) {
        process(std::move(line));
    }

    return 0;
}
