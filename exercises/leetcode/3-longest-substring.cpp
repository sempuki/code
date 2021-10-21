#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <numeric>
#include <set>
#include <utility>
#include <vector>

class Solution {
 public:
  int lengthOfLongestSubstring(std::string s) {
    size_t substr_max = 0;

    for (size_t i = 0; i < s.size(); ++i) {
      size_t substr_size = 1;

      for (size_t j = i + 1; j < s.size(); ++j) {
        auto substr_begin = s.begin() + i;
        auto substr_end = s.begin() + j;
        if (std::find(substr_begin, substr_end, s[j]) == substr_end) {
          ++substr_size;
        } else {
          break;
        }
      }

      substr_max = std::max(substr_max, substr_size);
    }

    return substr_max;
  }
};

int main() {
  auto example1 = "abcabcbb";
  auto example2 = "bbbbb";
  auto example3 = "pwwkew";
  auto example4 = "";

  std::cout << Solution().lengthOfLongestSubstring(example1) << std::endl;
  std::cout << Solution().lengthOfLongestSubstring(example2) << std::endl;
  std::cout << Solution().lengthOfLongestSubstring(example3) << std::endl;
  std::cout << Solution().lengthOfLongestSubstring(example4) << std::endl;
  return 0;
}
