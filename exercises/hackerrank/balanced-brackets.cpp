#include <iostream>
#include <stack>
#include <cassert>
using namespace std;

char flip(char ch) {
    switch (ch) {
    case '[': return ']';
    case '{': return '}';
    case '(': return ')';
    case ']': return '[';
    case '}': return '{';
    case ')': return '(';
    }
    assert(false);
}

int main() {
    size_t n;
    cin >> n;
    while(n--) {
        bool fail = false;
        std::stack<char> expected;
        std::string input;
        cin >> input;
        for (auto ch : input) {
            switch (ch) {
            case '[':
            case '{':
            case '(':
                expected.push(flip(ch));
                break;

            case ']':
            case '}':
            case ')':
                if (!expected.empty() && expected.top() == ch) {
                    expected.pop();
                } else {
                    fail = true;
                }
                break;
            }

            if (fail) {
                break;
            }
        }

        cout << (expected.empty() && !fail ? "YES" : "NO") << endl;
    }

    return 0;
}
