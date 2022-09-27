#include <iostream>
#include <utility>
#include <memory>
#include <array>
#include <cassert>

template <typename T, size_t N>
struct CircularBuffer {
  void push(T value) {
    buf[next] = std::move(value);
    next = (next + 1) % N;
    size = std::min(size + 1, N);
  }

  T pop() {
    assert(size);
    auto last = N + next - size;
    size -= 1;
    return std::move(buf[last % N]);
  }

  void print() const {
    auto last = N + next - size;
    for (size_t i = 0; i < size; ++i) {
      std::cout << buf[(last + i) % N] << ", ";
    }
    std::cout << "\n";
  }
  size_t next = 0;
  size_t size = 0;
  std::array<T, N> buf;
};

int main() {
  CircularBuffer<int, 3> b;
  b.print();  // []
  b.push(1);
  b.print();  // [1, ]
  b.push(2);
  b.print();  // [1, 2, ]
  b.push(3);
  b.print();  // [1, 2, 3, ]
  b.push(4); 
  b.print();  // [2, 3, 4, ]
  std::cout << b.pop() << "\n";  // 2
  b.print();  // [3, 4, ]
  std::cout << b.pop() << "\n";  // 3
  b.print();  // [4, ]
  b.push(5);
  b.print();  // [4, 5, ]

  CircularBuffer<std::unique_ptr<int>, 3> u;
  auto p = std::make_unique<int>(5);
  u.push(std::move(p));
}
