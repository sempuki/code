#include <cassert>
#include <iostream>
#include <memory>
#include <queue>
#include <stack>
#include <utility>

struct TreeNode {
  ~TreeNode() {
    delete left;
    delete right;
  }

  int val = 0;
  TreeNode *left = nullptr;
  TreeNode *right = nullptr;
};

void print(TreeNode *root) {
  struct Traversal {  // In-order left DFS
    TreeNode *node;
    bool expanded = false;
  };

  std::stack<Traversal> next;
  next.push({root});

  while (next.size()) {
    auto [node, expanded] = next.top();
    next.pop();

    if (expanded) {
      std::cout << node->val << ' ';
    } else {
      if (node->right) {
        next.push({node->right});
      }
      next.push({node, true});
      if (node->left) {
        next.push({node->left});
      }
    }
  }
}

class Solution {
 public:
  bool isCousins(TreeNode *root, int x, int y) {
    auto [x_heritage, y_heritage] = find(root, x, y);
    return x_heritage.depth == y_heritage.depth &&
           x_heritage.parent != y_heritage.parent;
  }

  struct Heritage {
    int parent = 0;
    int depth = 0;
  };

  std::pair<Heritage, Heritage> find(TreeNode *root, int x, int y) {
    std::pair<Heritage, Heritage> result;

    struct Traversal {  // Pre-order left BFS
      TreeNode *node;
      int parent = 0;
      int depth = 0;
    };

    std::queue<Traversal> next;
    next.push({root, 0});
    int found = 0;

    while (next.size() && found != 2) {
      auto [node, parent, depth] = next.front();
      next.pop();

      if (node->val == x) {
        result.first = {parent, depth};
        found++;
      }

      if (node->val == y) {
        result.second = {parent, depth};
        found++;
      }

      if (node->left) {
        next.push({node->left, node->val, depth + 1});
      }

      if (node->right) {
        next.push({node->right, node->val, depth + 1});
      }
    }

    return result;
  }
};

int main() {
  auto example1 =
      new TreeNode{1,  //
                   new TreeNode{2, new TreeNode{4}}, new TreeNode{3}};

  auto example2 = new TreeNode{1,  //
                               new TreeNode{2, nullptr, new TreeNode{4}},
                               new TreeNode{3, nullptr, new TreeNode{5}}};

  auto example3 =
      new TreeNode{1,  //
                   new TreeNode{2, nullptr, new TreeNode{4}}, new TreeNode{3}};

  Solution soln;
  std::cout << soln.isCousins(example1, 4, 3) << std::endl;
  std::cout << soln.isCousins(example2, 5, 4) << std::endl;
  std::cout << soln.isCousins(example3, 2, 3) << std::endl;
  return 0;
}
