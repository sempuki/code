#include <stack>

#include "dump.hpp"

using namespace std;

struct TreeNode {
  int val = 0;
  TreeNode *left = nullptr;
  TreeNode *right = nullptr;
  TreeNode(int x) : val(x) {}
  TreeNode(int x, TreeNode *l, TreeNode *r) : val(x), left(l), right(r) {}
};

class Solution {
 public:
  bool isValidBST(TreeNode *root) {
    bool valid = true;
    if (root) {
      stack<TreeNode *> next;
      next.push(root);

      while (valid && next.size()) {
        auto curr = next.top();
        next.pop();

        if (valid && curr->left) {
          valid = curr->left->val < curr->val;
          next.push(curr->left);
        }

        if (valid && curr->right) {
          valid = curr->right->val > curr->val;
          next.push(curr->right);
        }
      }
    }

    return valid;
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
