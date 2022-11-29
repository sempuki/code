#include <algorithm>
#include <utility>
#include <vector>

#include "dump.hpp"

using namespace std;

class Solution {
 public:
  void rotate1(vector<int>& nums, int k) {
    if (nums.size() > 1 && k > 0) {
      k %= nums.size();

      std::vector<int> shifted(std::prev(nums.end(), k), nums.end());

      auto shift_inp = std::next(nums.rbegin(), k);
      auto shift_out = nums.rbegin();
      auto shift_end = nums.rend();
      for (; shift_inp != shift_end; ++shift_inp, ++shift_out) {
        swap(*shift_inp, *shift_out);
      }

      std::copy(shifted.begin(), shifted.end(), nums.begin());
    }
  }

  void rotate(vector<int>& nums, int k) {
    std::rotate(nums.rbegin(), nums.rbegin() + k, nums.rend());
  }
};

int main() {
  auto ex1 = vector<int>{1, 2, 3, 4, 5, 6, 7};
  auto ex2 = vector<int>{-1, -100, 3, 99};
  auto ex3 = vector<int>{1, 2};

  Solution s{};
  s.rotate(ex1, 3);
  s.rotate(ex2, 2);
  s.rotate(ex3, 3);

  dump(ex1);
  dump(ex2);
  dump(ex3);
  return 0;
}
