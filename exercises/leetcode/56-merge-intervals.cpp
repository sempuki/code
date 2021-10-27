#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

namespace {
void print(const std::vector<std::vector<int>>& m) {
  std::cout << '[';

  bool first_row = true;
  for (size_t i = 0; i < m.size(); ++i) {
    if (first_row) {
      first_row = false;
    } else {
      std::cout << ',';
    }

    std::cout << '[';
    bool first_col = true;
    for (size_t j = 0; j < m[i].size(); ++j) {
      if (first_col) {
        first_col = false;
      } else {
        std::cout << ',';
      }

      std::cout << m[i][j];
    }
    std::cout << ']';
  }

  std::cout << "]\n";
}
}  // namespace

class Solution {
 public:
  std::vector<std::vector<int>> merge(
      std::vector<std::vector<int>>& intervals) {
    std::vector<std::vector<int>> result;

    if (intervals.size() > 1) {
      std::sort(intervals.begin(), intervals.end());  // Lexicographical order

      auto curr = intervals.begin();
      auto end = intervals.end();
      while (curr != end) {
        curr = std::find_if(curr, end, [](auto&& _) { return _.size(); });
        if (curr != end) {
          for (auto next = curr + 1; next != end; ++next) {
            if (next->front() <= curr->back()) {
              curr->back() = std::max(curr->back(), next->back());
              next->clear();
            }
          }
          curr++;
        }
      }
    }

    for (auto& interval : intervals) {
      if (interval.size()) {
        result.emplace_back(std::move(interval));
      }
    }
    intervals.clear();

    return result;
  }
};

int main() {
  auto example1 =
      std::vector<std::vector<int>>{{1, 3}, {2, 6}, {8, 10}, {15, 18}};
  auto example2 = std::vector<std::vector<int>>{{1, 4}, {4, 5}};
  auto example3 = std::vector<std::vector<int>>{{1, 4}, {5, 6}};
  auto example4 = std::vector<std::vector<int>>{{1, 4}, {2, 3}};

  print(Solution().merge(example1));
  print(Solution().merge(example2));
  print(Solution().merge(example3));
  print(Solution().merge(example4));

  return 0;
}
