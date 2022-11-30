#include <algorithm>
#include <utility>
#include <vector>

#include "dump.hpp"

using namespace std;

class Solution {
 public:
  vector<int> twoSum(vector<int>& nums, int target) {
    vector<int> result;
    if (nums.size() > 1) {
      vector<pair<int, size_t>> value_to_index;
      for (size_t i = 0; auto e : nums) {
        value_to_index.emplace_back(e, i++);
      }

      sort(value_to_index.begin(), value_to_index.end());
      auto lower = value_to_index.begin();
      auto upper = value_to_index.end();

      for (; lower != upper; lower++) {
        auto remain = target - lower->first;
        auto index = lower->second;
        auto found = find_if(lower, upper, [remain, index](auto&& p) {
          return p.first == remain && p.second != index;
        });
        if (found != value_to_index.end()) {
          result.push_back(lower->second);
          result.push_back(found->second);
          break;
        }
      }
    }
    return result;
  }
};

int main() {
  auto ex1 = vector<int>{2, 7, 11, 15};
  auto ex2 = vector<int>{3, 2, 4};
  auto ex3 = vector<int>{3, 3};
  auto ex4 = vector<int>{-1, -2, -3, -4, -5};
  auto ex5 = vector<int>{-10, -1, -18, -19};

  Solution s{};
  dump(s.twoSum(ex1, 9));
  dump(s.twoSum(ex2, 6));
  dump(s.twoSum(ex3, 6));
  dump(s.twoSum(ex4, -8));
  dump(s.twoSum(ex5, -19));

  return 0;
}
