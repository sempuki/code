#include <utility>
#include <vector>

#include "dump.hpp"

using namespace std;

class Solution {
 public:
  vector<int> intersect(vector<int>& nums1, vector<int>& nums2) {
    vector<int> result;
    vector<std::pair<int, int>> fastset(1001);
    for (auto e1 : nums1) {
      fastset[e1].first++;
    }
    for (auto e2 : nums2) {
      fastset[e2].second++;
    }
    for (size_t e = 0; auto&& p : fastset) {
      if (p.first && p.second) {
        result.insert(result.end(), min(p.first, p.second), e);
      }
      e++;
    }
    return result;
  }
};

int main() {
  auto ex1a = vector<int>{1, 2, 2, 1};
  auto ex1b = vector<int>{2, 2};
  auto ex2a = vector<int>{4, 9, 5};
  auto ex2b = vector<int>{9, 4, 9, 8, 4};

  Solution s{};
  dump(s.intersect(ex1a, ex1b));
  dump(s.intersect(ex2a, ex2b));

  return 0;
}
