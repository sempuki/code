// let $CXXFLAGS='--std=c++11'
//   Copyright: Ryan McDougall

#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <cassert>
#include <cctype>
#include <memory>
#include <stack>
#include <vector>


namespace {

int char_to_index(char ch) {
  return ch - 'a';
}

}  // namespace

class Trie {
 public:
  void insert(const std::string &word) {
    Node *node = &root_;
    for (char ch : word) {
      if (!node->children) {  // grow branch
        node->children = std::make_unique<Node::Children>();
      }
      node = &(node->children->at(char_to_index(ch)));
    }

    if (node && node != &root_) {
      node->is_word = true;
    }
  }

  bool contains(const std::string &word) const {
    const Node *node = &root_;
    for (char ch : word) {
      if (!node->children) {
        return false;
      }
      node = &(node->children->at(char_to_index(ch)));
    }

    return node && node->is_word;
  }

  bool is_compound(const std::string &word) const {
    bool compound = false;

    std::stack<std::pair<size_t, std::string>> stack;
    stack.push(std::make_pair(0, word));

    while (!compound && stack.size()) {
      const auto count = std::move(stack.top().first);
      const auto word = std::move(stack.top().second);
      stack.pop();

      const Node *node = &root_;
      for (size_t i = 0; node && i < word.size(); ++i) {
        if (node->is_word) {  // check remaining substring
          stack.push(std::make_pair(count+1, word.substr(i)));
        }
        node = node->children ?
          &(node->children->at(char_to_index(word[i]))) : nullptr;
      }
      compound = node && node->is_word && count > 0;
    }

    return compound;
  }

 private:
  struct Node {
    static constexpr size_t AlphabetSize = 26;
    using Children = std::array<Node, AlphabetSize>;
    std::unique_ptr<Children> children;
    bool is_word = false;
  };

  Node root_;

  friend std::ostream &operator<<(std::ostream &out, const Trie &table);
  friend std::vector<std::string> join(const std::string &prefix, const Trie::Node *node);
};


std::vector<std::string> join(const std::string &prefix, const Trie::Node *node) {
  std::vector<std::string> words;
  assert(node && "Invalid node");

  if (node->is_word) {
    words.push_back(prefix);
  }

  if (node->children) {
    for (size_t n = 0; n < node->children->size(); ++n) {
      auto ch = static_cast<char>(n + 'a');
      auto list = join(prefix + ch, &node->children->at(n));
      words.insert(end(words), begin(list), end(list));
    }
  }

  return words;
}

std::ostream &operator<<(std::ostream &out, const Trie &table) {
  auto words = join("", &table.root_);
  for (auto &&word : words) {
    std::cout << word << std::endl;
  }
  return out;
}



int main (int argc, char **argv)
{
    if (argc != 2) {
        std::cout << "input must be a valid filename" << std::endl;
        return -1;
    }

    std::ifstream file{argv[1]};
    if (!file) {
        std::cout << "input must be a file" << std::endl;
        return -2;
    }

    // We can do a bulk read into a buffer and parse in batches if I/O bound.
    // Memory is cheap and we can avoid heap fragmentation and copies by
    // reserving a large enough word buffer. C++11 compliant string impl
    // use SSO (>16 bytes on 64bit systems) so we should not need the heap
    // for input afterward.

    std::vector<std::string> words;
    words.reserve(1024 * 1024);  // avoid growth

    std::string word;
    while (std::getline(file, word)) {
      if (word.back() == '\r') {
        word.pop_back();
      }
      if (word.size()) {
        words.emplace_back(std::move(word));
      }
    }

    Trie trie;
    for (const auto &word : words) {
      trie.insert(word);
    }

    std::string longest_compound;
    std::string next_longest_compound;
    size_t compound_count = 0;

    for (const auto &word : words) {
      if (trie.is_compound(word)) {
        if (word.size() > longest_compound.size()) {
          next_longest_compound = std::move(longest_compound);
          longest_compound = word;
        }
        compound_count++;
      }
    }

    std::cout << longest_compound << std::endl;
    std::cout << next_longest_compound << std::endl;
    std::cout << compound_count << std::endl;

    return 0;
}
