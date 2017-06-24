#include <cassert>
#include <iostream>
#include <list>
#include <sstream>
using namespace std;

struct Token {
    enum class Type {
        none,
        num,
        op,
    };

    Type type = Type::none;
    int64_t value = 0;
};

int main() {
    string input;
    cin >> input;

    istringstream stream{input};
    list<Token> tokens;

    while (stream) {
        tokens.emplace_back();
        tokens.back().type = Token::Type::num;
        stream >> tokens.back().value;

        char ch = stream.get();
        if (ch == '+') {
            tokens.emplace_back();
            tokens.back().type = Token::Type::op;
            tokens.back().value = 1;
        } else if (ch == '*') {
            tokens.emplace_back();
            tokens.back().type = Token::Type::op;
            tokens.back().value = 2;
        } else {
            break;
        }
    }

    {  // Multiplication pass
        auto iter = tokens.begin();
        auto last = --tokens.end();
        while (iter != last) {
            auto lhs = iter++;
            auto op = iter++;
            if (op->type == Token::Type::op && op->value == 2) {
                iter->value *= lhs->value;
                tokens.erase(lhs, iter);
            }
        }
    }

    {  // Addition pass
        auto iter = tokens.begin();
        auto last = --tokens.end();
        while (iter != last) {
            auto lhs = iter++;
            auto op = iter++;
            if (op->type == Token::Type::op && op->value == 1) {
                iter->value += lhs->value;
                tokens.erase(lhs, iter);
            }
        }
    }

    cout << tokens.front().value << endl;
}
