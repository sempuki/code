#include <algorithm>
#include <map>
#include <vector>

#include "dump.hpp"

using namespace std;

class Solution {
 public:
  vector<int> twoSum(vector<int>& nums, int target) {
    vector<int> result;
    if (nums.size() > 1) {
      multimap<int, size_t> value_to_index;
      for (size_t i = 0; auto e : nums) {
        value_to_index.emplace(e, i++);
      }

      auto lower = value_to_index.begin();
      auto upper = value_to_index.end();
      for (; lower != upper && result.empty(); lower++) {
        auto remain = target - lower->first;
        auto index = lower->second;
        auto found = value_to_index.equal_range(remain);
        if (found.first != value_to_index.end()) {
          switch (distance(found.first, found.second)) {
            case 0u:  // no matches
              break;
            case 1u:  // one match
              if (found.first->second != index) {
                result.push_back(found.first->second);
                result.push_back(lower->second);
              }
              break;
            default:  // N matches
              auto other =
                find_if(found.first, found.second, [index](auto&& p) { return p.second != index; });
              if (other != found.second) {
                result.push_back(other->second);
                result.push_back(lower->second);
              }
              break;
          }
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
  auto ex6 = vector<int>{2, 4, 11, 3};

  Solution s{};
  dump(s.twoSum(ex1, 9));
  dump(s.twoSum(ex2, 6));
  dump(s.twoSum(ex3, 6));
  dump(s.twoSum(ex4, -8));
  dump(s.twoSum(ex5, -19));
  dump(s.twoSum(ex6, 6));

  return 0;
}
