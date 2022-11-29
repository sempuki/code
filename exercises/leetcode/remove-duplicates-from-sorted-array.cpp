#include <iostream>
#include <iterator>
#include <vector>

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
  cout << s.removeDuplicates(ex1) << "\n";
  cout << s.removeDuplicates(ex2) << "\n";

  return 0;
}
