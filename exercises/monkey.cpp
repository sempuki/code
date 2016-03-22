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

struct Point {
  int x = 0;
  int y = 0;
};

constexpr int64_t absolute(int64_t number) {
  // branchless: multiply number by its sign
  return ((0 < number) - (number < 0)) * number;
}

constexpr uint64_t sum_digits(uint64_t number) {
  uint64_t total = 0;
  while (number > 0) {
    total += number % 10;
    number /= 10;
  }
  return total;
}

constexpr bool is_valid(const Point &point) {
  return (sum_digits(absolute(point.x)) + sum_digits(absolute(point.y))) <= 19;
}

static_assert(absolute(0) == 0, "test abs of zero");
static_assert(absolute(12345) == 12345, "test abs of positive");
static_assert(absolute(-12345) == 12345, "test abs of negative");
static_assert(sum_digits(123456789) == 45, "test sum_digits 1-9");
static_assert(is_valid({-5, -7}) == true, "test accessible point");
static_assert(is_valid({59, 79}) == false, "test inaccessible point");

int main() {
}
