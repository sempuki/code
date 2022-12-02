#include <map>
#include <stack>

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
      stack<TreeNode *> next;
      next.push(root);

      map<TreeNode *, TreeNode *> parents;
      parents[root] = nullptr;

      while (next.size()) {
        auto curr = next.top();
        next.pop();

        if (curr->left) {
          next.push(curr->left);
          parents[curr->left] = curr;
        }

        if (curr->right) {
          next.push(curr->right);
          parents[curr->right] = curr;
        }
      }

      for (auto &&node_to_parent : parents) {
        auto *curr = node_to_parent.first;
        auto *parent = node_to_parent.second;
        auto value = curr->val;
        while (curr && parent) {
          if ((curr == parent->left && value >= parent->val) ||
              (curr == parent->right && value <= parent->val)) {
            return false;
          }
          curr = parent;
          parent = parents[curr];
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
