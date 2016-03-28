// Copyright Ryan McDougall (2016)
//   let $CPPFLAGS='--std=c++14'
//
// There is a monkey which can walk around on a planar grid. The monkey can
// move one space at a time left, right, up or down. That is, from (x, y) the
// monkey can go to (x+1, y), (x-1, y), (x, y+1), and (x, y-1). Points where
// the sum of the digits of the absolute value of the x coordinate plus the sum
// of the digits of the absolute value of the y coordinate are lesser than or
// equal to 19 are accessible to the monkey. For example, the point (59, 79) is
// inaccessible because 5 + 9 + 7 + 9 = 30, which is greater than 19. Another
// example: the point (-5, -7) is accessible because abs(-5) + abs(-7) = 5 + 7
// = 12, which is less than 19. How many points can the monkey access if it
// starts at (0, 0), including (0, 0) itself? 

#include <iostream>

#include <cassert>
#include <array>
#include <algorithm>
#include <queue>

constexpr int64_t absolute(int64_t number) {
  // return ((0 < number) - (number < 0)) * number;  // branchless
  return number >= 0 ? number : -number;
}

struct Point {
  int x = 0;
  int y = 0;
};

template <size_t M, size_t N>
class Table {
 public:
  Table() {
    for (auto &&row : visited_) {
      std::fill(begin(row), end(row), false);
    }
  }

  void visit(Point const &point) {
    auto x = absolute(point.x);
    auto y = absolute(point.y);

    assert(x >= 0 && x < rows() && "Out of bounds");
    assert(y >= 0 && y < cols() && "Out of bounds");

    visited_[x][y] = true;
  }

  bool visited(Point const &point) const { 
    auto x = absolute(point.x);
    auto y = absolute(point.y);

    assert(x >= 0 && x < rows() && "Out of bounds");
    assert(y >= 0 && y < cols() && "Out of bounds");

    return visited_[x][y];
  }

 public:
  size_t rows() const { return M; }
  size_t cols() const { return N; }

 private:
  std::array<std::array<bool, N>, M> visited_;
};

template <size_t M, size_t N>
void operator<<(std::ostream &out, Table<M,N> const &table) {
  for (int i = 0; i < table.rows(); ++i) {
    int count = 0;
    for (int j = 0; j < table.cols(); ++j) {
      if (table.visited({i, j})) {
        std::cout << '(' << i << ',' << j << "), ";
        count++;
      }
    }
    if (count) {
      std::cout << std::endl;
    }
  }
}

constexpr uint64_t sum_digits(uint64_t number) {
  uint64_t total = 0;
  while (number > 0) {
    total += number % 10;
    number /= 10;
  }
  return total;
}

constexpr size_t LIMIT = 19;

constexpr bool is_valid(const Point &point, size_t limit = LIMIT) {
  return (sum_digits(absolute(point.x)) + sum_digits(absolute(point.y))) <= limit;
}

constexpr size_t largest_extent(size_t limit = LIMIT) {
  int size = 0;
  while (sum_digits(++size) <= limit) {
  }
  return size;
}

static_assert(absolute(0) == 0, "test abs of zero");
static_assert(absolute(12345) == 12345, "test abs of positive");
static_assert(absolute(-12345) == 12345, "test abs of negative");
static_assert(sum_digits(123456789) == 45, "test sum_digits 1-9");
static_assert(is_valid({-5, -7}, 19) == true, "test accessible point");
static_assert(is_valid({59, 79}, 19) == false, "test inaccessible point");
static_assert(largest_extent(19) == 299, "test table size");


int main() {
  size_t constexpr N = largest_extent(LIMIT);
  std::cout << "largest: " << N << std::endl;

  auto quadrant = std::make_unique<Table<N,N>>();
  uint64_t count = 0;

  std::queue<Point> points;  // breadth-first
  points.push({0, 0});
  while (!points.empty()) {
    auto &&point = points.front(); points.pop();
    if (is_valid(point) && !quadrant->visited(point)) {
      points.push({point.x + 1, point.y});
      points.push({point.x - 1, point.y});
      points.push({point.x, point.y + 1});
      points.push({point.x, point.y - 1});
      quadrant->visit(point);
      count++;
    }
  }

  // 4 quadrants symmetric to the first (don't over count)
  std::cout << "count: " << (4 * (count - N) + 1) << std::endl;
  // std::cout << "table: \n" << *quadrant;
}
