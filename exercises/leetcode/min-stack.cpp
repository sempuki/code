#include <algorithm>
#include <stack>

#include "dump.hpp"

using namespace std;

class MinStack {
 public:
  MinStack() {}

  void push(int val) {
    stack_.push(make_pair(val, stack_.size() ? min(val, stack_.top().second) : val));
  }

  void pop() {
    stack_.pop();  //
  }

  int top() {
    return stack_.top().first;  //
  }

  int getMin() {
    return stack_.top().second;  //
  }

 private:
  stack<pair<int, int>> stack_;
};

int main() {
  MinStack minStack{};
  minStack.push(-2);
  minStack.push(0);
  minStack.push(-3);
  dump(minStack.getMin());  // return -3
  minStack.pop();
  dump(minStack.top());     // return 0
  dump(minStack.getMin());  // return -2

  return 0;
}
