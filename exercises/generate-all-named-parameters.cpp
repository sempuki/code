#include <cassert>
#include <iostream>
#include <map>
#include <stack>
#include <vector>

std::stack<std::vector<size_t>> generate_indicies(
  const std::map<std::string, std::vector<std::string>>& input) {
  std::stack<std::vector<size_t>> result;

  for (auto&& name_to_params : input) {
    if (result.empty()) {
      for (size_t n = 0; n < name_to_params.second.size(); ++n) {
        result.push({n});
      }
    } else {
      std::stack<std::vector<size_t>> other;
      while (result.size()) {
        auto partial = std::move(result.top());
        result.pop();

        for (size_t n = 0; n < name_to_params.second.size(); ++n) {
          auto copy = partial;
          copy.push_back(n);
          other.push(std::move(copy));
        }
      }
      result = std::move(other);
    }
  }

  return result;
}

int main() {
  std::vector<std::string> names = {"name1", "name2", "name3"};
  std::map<std::string, std::vector<std::string>> input = {
    {"name1", {"this", "that", "other"}},  //
    {"name2", {"red", "green"}},           //
    {"name3", {"1", "2", "3", "4", "5"}},  //
  };

  auto result = generate_indicies(input);
  for (size_t param_count = 0; result.size(); ++param_count) {
    auto params = std::move(result.top());
    result.pop();

    assert(params.size() == input.size());
    std::cout << "== parameterization " << param_count << " : { ";
    for (size_t i = 0; i < params.size(); ++i) {
      std::cout << input[names[i]][params[i]] << ", ";
    }
    std::cout << "}\n ";
  }
}
