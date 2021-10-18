#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

std::unique_ptr<std::istream, void (*)(std::istream*)> choose_input_stream(
    int argc, char** argv) {
  if (argc == 1) {
    return {&std::cin, [](std::istream*) {}};
  } else if (argc == 2) {
    return {new std::ifstream{argv[1]}, [](std::istream* p) { delete p; }};
  } else {
    return {nullptr, nullptr};
  }
}

template <uint64_t BitSize>
class BitArray {
 public:
  BitArray()
      : data_{std::make_unique<std::array<uint64_t, (BitSize / 64ULL)>>()} {
    data_->fill(0);
  }

  bool visit(uint64_t position) {
    uint64_t block = position >> 6ULL;
    uint64_t offset = 1ULL << (position & 0x3FULL);

    bool visited = (*data_)[block] & offset;
    (*data_)[block] |= offset;

    return visited;
  }

 private:
  std::unique_ptr<std::array<uint64_t, (BitSize / 64ULL)>> data_;
};

int main(int argc, char** argv) {
  auto input = choose_input_stream(argc, argv);

  constexpr int64_t M = (1LL << 31);
  int64_t N, S, P, Q;
  if ((*input >> N) &&  // Size of array
      (*input >> S) &&  // Initial value
      (*input >> P) &&  // Prod factor
      (*input >> Q)) {  // Sum term
    size_t count = 0;

    if (N) {
      int64_t curr = S % M;
      int64_t next = 0;

      BitArray<M> visited;

      for (int64_t i = 0; i < N; ++i, ++count) {
        if (i != 0) {
          curr = next;
        }

        auto value = visited.visit(curr);
        if (value) {
          break;  // Cycle
        }

        next = (curr * P + Q) % M;
      }
    }

    std::cout << count << "\n";
  } else {
    std::cerr << "Bad input\n";
  }
}
