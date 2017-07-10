/* Hidden stub code will pass a root argument to the function below. Complete the function to solve the challenge. Hint: you may want to write one or more helper functions.  

   The Node struct is defined as follows:
   struct Node {
       int data;
       Node* left;
       Node* right;
   }
*/
bool node_less(Node* node, int value) {
    if (!node) {
        return true;
    } else {
        return node->data < value
            && node_less(node->left, value)
            && node_less(node->right, value)
            && node_less(node->left, node->data);
    }
}

bool node_greater(Node* node, int value) {
    if (!node) {
        return true;
    } else {
        return node->data > value
            && node_greater(node->left, value)
            && node_greater(node->right, value)
            && node_greater(node->right, node->data);
    }
}

bool checkBST(Node* root) {
    if (!root) {
        return true;
    } else {
        return node_less(root->left, root->data)
            && node_greater(root->right, root->data);
    }
}

