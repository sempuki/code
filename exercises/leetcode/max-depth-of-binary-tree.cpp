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
  int maxDepth(TreeNode *root) {
    int depth = 0;
    if (root) {
      stack<pair<int, TreeNode *>> next;
      next.push(make_pair(depth + 1, root));

      while (next.size()) {
        auto curr = next.top();
        next.pop();
        if (curr.second->left) {
          next.push(make_pair(curr.first + 1, curr.second->left));
        }
        if (curr.second->right) {
          next.push(make_pair(curr.first + 1, curr.second->right));
        }
        depth = max(depth, curr.first);
      }
    }

    return depth;
  }
};

int main() {
  auto *ex1 = new TreeNode(3,                              //
                           new TreeNode(9),                //
                           new TreeNode(20,                //
                                        new TreeNode(15),  //
                                        new TreeNode(7)));
  auto *ex2 = new TreeNode(1,        //
                           nullptr,  //
                           new TreeNode(2));
  Solution s{};
  dump(s.maxDepth(ex1));
  dump(s.maxDepth(ex2));
  return 0;
}
