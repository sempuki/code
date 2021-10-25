#include <cassert>
#include <iostream>
#include <utility>
#include <vector>

namespace {
template <typename I>
auto find_merged(I &iter1, const I &end1, I &iter2, const I &end2) {
  auto merged = iter1;
  if (iter1 != end1 && iter2 != end2) {
    if (*iter1 < *iter2) {
      merged = iter1++;
    } else {
      merged = iter2++;
    }
  } else if (iter1 != end1) {
    merged = iter1++;
  } else if (iter2 != end2) {
    merged = iter2++;
  } else {
    assert(false);
  }
  return merged;
}
}  // namespace

class Solution {
 public:
  double findMedianSortedArrays(std::vector<int> &nums1,
                                std::vector<int> &nums2) {
    size_t combined_size = nums1.size() + nums2.size();
    size_t mid_point = combined_size / 2;
    bool is_even = (combined_size % 2) == 0;

    auto iter1 = nums1.begin();
    auto end1 = nums1.end();
    auto iter2 = nums2.begin();
    auto end2 = nums2.end();
    auto merged = iter1;

    for (size_t i = 0; i < mid_point; ++i) {
      merged = find_merged(iter1, end1, iter2, end2);
    }

    double mid_value = 0.0;
    if (is_even) {
      mid_value = *merged;
      merged = find_merged(iter1, end1, iter2, end2);
      mid_value += *merged;
      mid_value /= 2.0;
    } else {
      merged = find_merged(iter1, end1, iter2, end2);
      mid_value = *merged;
    }

    return mid_value;
  }

 private:
};

int main() {
  std::pair<std::vector<int>, std::vector<int>> example1{{1, 3}, {2}};
  std::pair<std::vector<int>, std::vector<int>> example2{{1, 2}, {3, 4}};
  std::pair<std::vector<int>, std::vector<int>> example3{{0, 0}, {0, 0}};
  std::pair<std::vector<int>, std::vector<int>> example4{{}, {1}};
  std::pair<std::vector<int>, std::vector<int>> example5{{2}, {}};

  std::cout << Solution().findMedianSortedArrays(example1.first,
                                                 example1.second)
            << std::endl;
  std::cout << Solution().findMedianSortedArrays(example2.first,
                                                 example2.second)
            << std::endl;
  std::cout << Solution().findMedianSortedArrays(example3.first,
                                                 example3.second)
            << std::endl;
  std::cout << Solution().findMedianSortedArrays(example4.first,
                                                 example4.second)
            << std::endl;
  std::cout << Solution().findMedianSortedArrays(example5.first,
                                                 example5.second)
            << std::endl;
  return 0;
}
