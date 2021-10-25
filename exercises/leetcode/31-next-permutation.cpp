#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void print(std::vector<int>& nums) {
  for (auto i : nums) {
    std::cout << i << ',';
  }
  std::cout << "\n";
}
}  // namespace

class Solution {
 public:
  // 1. Find the least significant (end -> begin) lexigraphically ordered pair.
  //    The more significant value is the "pivot".
  // 2. Find the least significant value larger than the "pivot", and swap
  //    (making it lexigraphically larger).
  // 3. "Wrap" the non-pivot values to a lesser lexigraphical value by
  //    reversing them.

  void nextPermutation(std::vector<int>& nums) {
    if (nums.size() > 1) {
      auto swapped = false;

      // Use of reverse iterators also reverses binary operation.
      auto prev =
          std::adjacent_find(nums.rbegin(), nums.rend(), std::greater<>());
      if (prev != nums.rend()) {
        auto pivot = std::next(prev);
        auto swap = std::find_if(nums.rbegin(), nums.rend(),
                                 [p = *pivot](auto v) { return v > p; });
        if (swap != nums.rend()) {
          std::iter_swap(pivot, swap);
          std::reverse(nums.rbegin(), pivot);
          swapped = true;
        }
      }

      if (!swapped) {
        std::reverse(nums.begin(), nums.end());
      }
    }
  }

  void nextPermutation1(std::vector<int>& nums) {
    if (nums.size() > 1) {
      auto swapped = false;
      auto first = nums.begin();
      auto last = --nums.end();
      auto pivot = last;

      while (pivot != first && !swapped) {
        auto prev = pivot--;

        if (*pivot < *prev) {
          auto swap = last;
          for (; !(*pivot < *swap); --swap) {
          }
          std::iter_swap(pivot, swap);
          std::reverse(prev, nums.end());
          swapped = true;
        }
      }

      if (!swapped) {
        std::reverse(nums.begin(), nums.end());
      }
    }
  }
};

int main() {
  std::vector<int> example1 = {1, 2, 3};
  std::vector<int> example2 = {3, 2, 1};
  std::vector<int> example3 = {1, 1, 5};
  std::vector<int> example4 = {1};

  Solution().nextPermutation(example1);
  print(example1);
  Solution().nextPermutation(example1);
  print(example1);
  Solution().nextPermutation(example1);
  print(example1);
  Solution().nextPermutation(example1);
  print(example1);
  Solution().nextPermutation(example1);
  print(example1);
  Solution().nextPermutation(example2);
  print(example2);
  Solution().nextPermutation(example3);
  print(example3);
  Solution().nextPermutation(example3);
  print(example3);
  Solution().nextPermutation(example4);
  print(example4);

  return 0;
}
