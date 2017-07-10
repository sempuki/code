#include <iostream>
#include <stack>
using namespace std;

int main() {
    std::stack<int> values;
    std::stack<std::pair<int, int>> maxes;
    maxes.emplace(0, 0);
    
    size_t n, command, value;
    cin >> n;
    while(n--) {
        cin >> command;
        switch (command) {
        case 1:
            cin >> value;
            if (value > maxes.top().first) {
                maxes.emplace(value, 1);
            } else {
                maxes.top().second += 1;
            }
            values.push(value);
            break;
        case 2:
            value = values.top();
            values.pop();
            maxes.top().second -= 1;
            if (maxes.top().second == 0) {
                maxes.pop();
            }
            break;
        case 3:
            cout << maxes.top().first << "\n";
            break;
        }
    }
    return 0;
}
