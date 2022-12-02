#include <stack>
#include <unordered_map>

#include "dump.hpp"

using namespace std;

struct TreeNode {
  int val = 0;
  TreeNode *left = nullptr;
  TreeNode *right = nullptr;
};

class Solution {
 public:
  bool isValidBST(TreeNode *root) {
    if (root) {
      struct Traversal {
        TreeNode *node = nullptr;
        bool expanded = false;
      };

      stack<Traversal> next;
      next.push({root});

      unordered_map<TreeNode *, TreeNode *> parents;
      parents[root] = nullptr;

      while (next.size()) {
        auto [node, expanded] = next.top();
        next.pop();

        if (expanded) {
          auto *curr = node;
          auto *parent = parents[node];
          while (curr && parent) {
            if ((curr == parent->left && node->val >= parent->val) ||
                (curr == parent->right && node->val <= parent->val)) {
              return false;
            }
            curr = parent;
            parent = parents[curr];
          }

          parents.erase(node);
        } else {
          next.push({node, true});

          if (node->right) {
            next.push({node->right});
            parents[node->right] = node;
          }
          if (node->left) {
            next.push({node->left});
            parents[node->left] = node;
          }
        }
      }
    }

    return true;
  }
};

int main() {
  auto *ex1 = new TreeNode(5,                             //
                           new TreeNode(1),               //
                           new TreeNode(4,                //
                                        new TreeNode(3),  //
                                        new TreeNode(6)));
  auto *ex2 = new TreeNode(2,  //
                           new TreeNode(1),
                           new TreeNode(3));
  auto *ex3 = new TreeNode(5,                             //
                           new TreeNode(4),               //
                           new TreeNode(6,                //
                                        new TreeNode(3),  //
                                        new TreeNode(7)));

  Solution s{};
  dump(s.isValidBST(ex1));
  dump(s.isValidBST(ex2));
  dump(s.isValidBST(ex3));
  return 0;
}
