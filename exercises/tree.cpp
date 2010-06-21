/* tree.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <stack>
#include <queue>

using namespace std;

template <typename T>
struct Node
{
    T value;

    Node *left;
    Node *right;

    Node (T v) : value (v) {}
};

template <typename T>
void print (T v) 
{ 
    cout << v << " "; 
}

// time complexity: asympotically linear; 
// O(b^d) where b is branching factor and d is depth;
// each node must be visited

// space complexity: O(n); each node is placed on the stack
// note: function is in preorder position for postorder, 
// place behind recursive calls
template <typename T, typename F>
void apply_r (Node <T> *n, F f)
{
    if (!n) return;

    f (n->value);

    apply_r (n->left, f);
    apply_r (n->right, f);
}


// space complexity: O(lgn); max stack size is the depth of the tree
// note: currently left depth first; for right depth first, reorder pushes
template <typename T, typename F>
void apply_dfs (Node <T> *n, F f)
{
    stack <Node <T> *> next;
    Node <T> *curr;

    next.push (n);

    while (next.size ())
    {
        curr = next.top ();
        next.pop ();

        if (curr)
        {
            f (curr->value);

            next.push (curr->right);
            next.push (curr->left);
        }
    }
}


// space complexity: O(n); max queue size is all children at max depth
// note: currently left breadth first; for right breadth first, reorder pushes
template <typename T, typename F>
void apply_bfs (Node <T> *n, F f)
{
    queue <Node <T> *> next;
    Node <T> *curr;

    next.push (n);

    while (next.size ())
    {
        curr = next.front ();
        next.pop ();

        if (curr)
        {
            f (curr->value);

            next.push (curr->left);
            next.push (curr->right);
        }
    }
}


//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    Node <int> *tree = new Node <int> (1);
    tree->left = new Node <int> (2);
    tree->right = new Node <int> (3);
    tree->left->left = new Node <int> (4);
    tree->left->right = new Node <int> (5);
    tree->right->left = new Node <int> (6);
    tree->right->right = new Node <int> (7);
    
    cout << "recursive: ";
    apply_r (tree, print<int>);
    cout << endl;

    cout << "depth first search: ";
    apply_dfs (tree, print<int>);
    cout << endl;

    cout << "breadth first search: ";
    apply_bfs (tree, print<int>);
    cout << endl;

    return 0;
}
