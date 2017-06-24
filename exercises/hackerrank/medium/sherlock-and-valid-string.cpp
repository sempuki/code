#include <cassert>
#include <iostream>
#include <sstream>
using namespace std;

int main() {
    string input;
    cin >> input;

    int64_t accumulator = 0;
    int64_t number = 0;
    string op;
    istringstream stream{input};

    stream >> accumulator;
    while (stream) {
        stream >> number;
        stream >> op;
        if (op == "+") {
            accumulator += number;
        } else if (op == "*") {
            accumulator *= number;
        } else {
            assert(false);
        }
    }
}
