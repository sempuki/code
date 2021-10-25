#include <algorithm>
#include <cassert>
#include <iostream>
#include <stack>
#include <vector>

namespace {
void print(const std::vector<std::vector<int>>& nums) {
  std::cout << "[\n";
  for (const auto& row : nums) {
    std::cout << "\t[";
    for (auto v : row) {
      std::cout << v << ',';
    }
    std::cout << "]\n";
  }
  std::cout << "]\n";
}
}  // namespace

class Solution {
 public:
  std::vector<std::vector<int>> combine(int n, int k) {
    std::vector<std::vector<int>> result;

    std::vector<int> values(n);
    std::generate_n(values.begin(), n, [i = 1]() mutable { return i++; });

    std::vector<int8_t> selection_mask(n);
    std::fill_n(selection_mask.begin(), n - k, 0);
    std::fill_n(selection_mask.begin() + n - k, k, 1);

    do {
      result.emplace_back();
      result.back().reserve(k);
      for (int i = 1; i <= n; ++i) {
        if (selection_mask[i - 1]) {
          result.back().push_back(i);
        }
      }
    } while (
        std::next_permutation(selection_mask.begin(), selection_mask.end()));

    return result;
  }
};

int main() {
  std::pair<int, int> example1 = {5, 3};

  print(Solution().combine(example1.first, example1.second));

  return 0;
}
