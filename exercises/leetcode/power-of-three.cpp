#include <algorithm>
#include <array>
#include <cmath>

#include "dump.hpp"

using namespace std;

class Solution {
 public:
  Solution() {
    pow3s[0] = 1;
    for (int i = 1; i < pow3s.size(); ++i) {
      pow3s[i] = pow3s[i - 1] * 3;
    }
  }
  bool isPowerOfThree(int n) {
    if (n > 0) {
      return find(pow3s.begin(), pow3s.end(), abs(n)) != pow3s.end();
    }
    return false;
  }
  array<int, 20> pow3s;
};

int main() {
  Solution s{};
  dump(s.isPowerOfThree(27));
  dump(s.isPowerOfThree(0));
  dump(s.isPowerOfThree(-1));
  return 0;
}
