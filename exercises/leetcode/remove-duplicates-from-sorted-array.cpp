#include <vector>

#include "dump.hpp"

using namespace std;

class Solution {
 public:
  int removeDuplicates(vector<int>& nums) {
    auto output = nums.begin();
    auto next_input = next(output);
    for (; next_input != nums.end(); ++next_input) {
      if (*output != *next_input) {
        ++output;
        *output = *next_input;
      }
    }
    return distance(nums.begin(), output) + 1;
  }
};

int main() {
  auto ex1 = vector<int>{1, 2, 2};
  auto ex2 = vector<int>{0, 0, 1, 1, 1, 2, 2, 3, 3, 4};

  Solution s{};
  dump(s.removeDuplicates(ex1));
  dump(s.removeDuplicates(ex2));

  return 0;
}
