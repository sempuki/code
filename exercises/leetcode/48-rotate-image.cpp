#include <iostream>
#include <vector>

namespace {
std::pair<int, int> map_index(int i, int j, int N) {
  return {N - j - 1, i};  // Inverse 90 deg rotation
}

void print(std::vector<std::vector<int>>& matrix) {
  const int N = matrix.size();
  std::cout << '[';

  bool first_row = true;
  for (int i = 0; i < N; ++i) {
    if (first_row) {
      first_row = false;
    } else {
      std::cout << ',';
    }

    std::cout << '[';
    bool first_col = true;
    for (int j = 0; j < N; ++j) {
      if (first_col) {
        first_col = false;
      } else {
        std::cout << ',';
      }

      std::cout << matrix[i][j];
    }
    std::cout << ']';
  }

  std::cout << "]\n";
}
}  // namespace

class Solution {
 public:
  void rotate(std::vector<std::vector<int>>& matrix) {
    const int N = matrix.size();

    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < N; ++j) {
        auto [ii, jj] = map_index(i, j, N);
      }
    }
  }
};

int main() {
  std::vector<std::vector<int>> example1 = {
      {1, 2, 3},
      {4, 5, 6},
      {7, 8, 9},
  };

  Solution().rotate(example1);
  print(example1);

  return 0;
}
