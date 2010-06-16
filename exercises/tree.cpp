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

// time and space complexity: linear 
// O(b^d) where b is branching factor and d is depth

// note: function is in preorder position
// for postorder, place behind recursive calls
template <typename T, typename F>
void apply_r (Node <T> *n, F f)
{
    if (!n) return;

    f (n->value);

    apply_r (n->left, f);
    apply_r (n->right, f);
}


// note: currently left depth first 
// for right depth first, reorder pushes
template <typename T, typename F>
void apply_dfs (Node <T> *n, F f)
{
    stack <Node <T> *> context;
    Node <T> *curr;

    context.push (n);

    while (context.size ())
    {
        curr = context.top ();
        context.pop ();

        if (curr)
        {
            f (curr->value);

            context.push (curr->right);
            context.push (curr->left);
        }
    }
}


// note: currently left breadth first 
// for right breadth first, reorder pushes
template <typename T, typename F>
void apply_bfs (Node <T> *n, F f)
{
    queue <Node <T> *> context;
    Node <T> *curr;

    context.push (n);

    while (context.size ())
    {
        curr = context.front ();
        context.pop ();

        if (curr)
        {
            f (curr->value);

            context.push (curr->left);
            context.push (curr->right);
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
