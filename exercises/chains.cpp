#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <stack>
#include <algorithm>

struct node {
  int first, last, children = 0;
  std::string word;

  node (const std::string &word) :
    first {word.front() - 'a'},
    last {word.back() - 'a'},
    word {word} {}

  bool operator==(const node &that) {
    return word == that.word;
  }
};

std::string solve(const std::vector<std::string> &list) {
  std::array<std::vector<node>, 26> nodes;

  for (const auto &word : list) {
    node node {word};
    nodes[node.first].push_back(node);
  }

  std::stack<node> stack;
  std::vector<node> path, answer;
  for (const auto &word : list) {
    auto start = node {word};
    stack.push(start);
    path.clear();

    while (!stack.empty()) {
      auto current = stack.top(); stack.pop();
      path.push_back(current);

      if (path.size() > answer.size()) {
        answer = path;
      }

      for (const auto &next : nodes[current.last]) {
        if (std::find(begin(path), end(path), next) == end(path)) {
          path.back().children++;
          stack.push(next);
        }
      }

      while (!path.empty() && !path.back().children) {
        path.pop_back();
        if (!path.empty()) {
          path.back().children--;
        }
      }
    }
  }

  std::string result;
  for (const auto &node : answer) {
    result += node.word;
    result += ", ";
  }

  return result;
}

// possible test generator
std::vector<std::string> wordlist() {
  std::vector<std::string> list;

  std::srand(std::time(0));
  const int N = (std::rand() % 30) + 5;
  for (int i=0; i < N; ++i) {
    std::string s;
    const int M = (std::rand() % 5) + 3;
    for (int j=0; j < M; ++j) {
      s += (std::rand() % 26) + 'a';
    }
    list.push_back(s);
  }

  return list;
}

int main() {
  // std::cout << solve({"soup", "sugar", "peas", "rice"}) << std::endl;
  // std::cout << solve({"cjz", "tojiv", "sgxf", "awonm", "fcv"}) << std::endl;
  // std::cout << solve({"ljhqi", "nrtxgiu", "jdtphez", "wosqm"}) << std::endl;

  auto list = wordlist();
  std::cout << "words: ";
  for (const auto &word : list) {
    std::cout << word << ", ";
  }
  std::cout << std::endl;
  std::cout << "solution: " << solve(list) << std::endl;
}
