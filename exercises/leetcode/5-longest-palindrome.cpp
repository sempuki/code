#include <iostream>
#include <string_view>

namespace {
bool is_palindrome(std::string_view s) {
  for (size_t i = 0; i < s.size() / 2; ++i) {
    if (s[i] != s[s.size() - i - 1]) {
      return false;
    }
  }
  return true;
}
}  // namespace

class Solution {
 public:
  std::string longestPalindrome(std::string s) {
    std::string_view palindrome{&s[0], 1};  // Always a palindrome of 1

    for (size_t i = 0; i < s.size(); ++i) {
      for (size_t j = s.size() - i; j; --j) {
        std::string_view substr(&s[i], j);
        if (substr.size() > palindrome.size() && is_palindrome(substr)) {
          palindrome = substr;
        }
      }
    }

    return std::string{palindrome};
  }
};

int main() {
  auto example1 = "babad";
  auto example2 = "cbbd";
  auto example3 = "a";
  auto example4 = "ac";
  auto example5 = "abcdbbfcba";

  std::cout << Solution().longestPalindrome(example1) << std::endl;
  std::cout << Solution().longestPalindrome(example2) << std::endl;
  std::cout << Solution().longestPalindrome(example3) << std::endl;
  std::cout << Solution().longestPalindrome(example4) << std::endl;
  std::cout << Solution().longestPalindrome(example5) << std::endl;
  return 0;
}
