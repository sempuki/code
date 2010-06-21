/* bst.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <stack>

using namespace std;

template <typename T>
class BinarySearchTree
{
    public:
        struct Node
        {
            T       value;
            Node    *left;
            Node    *right;

            Node (const T &v, Node *l = 0, Node *r = 0) : 
                value (v), left (l), right (r) {}
        };

        struct Result
        {
            Node    **link;
            Node    *node;

            Result (Node **l = 0, Node *n = 0) :
                link (l), node (n) {}
        };

    public:
        BinarySearchTree () : size_ (0), head_ (0) {}
        ~BinarySearchTree () {}

    public:
        Result find (const T &v)
        {
            Result r (0, head_);

            while (r.node)
            {
                if (v == r.node->value)
                {
                    return r;
                }
                else if (v < r.node->value)
                {
                    r.link = &(r.node->left);
                    r.node = r.node->left;
                }
                else 
                {
                    r.link = &(r.node->right);
                    r.node = r.node->right;
                }
            }

            return r;
        }

        Node *insert (const T &v)
        {
            Result r = find (v);
            Node *n = 0;

            if (r.node)
                n = r.node;

            else
            {
                n = new Node (v);
                ++ size_;

                if (r.link)
                    *(r.link) = n;
                else 
                    head_ = n;
            }
                
            return n;
        }

        Node *erase (const T &v)
        {
            Result r = find (v);
            Node *n = 0;

            if (r.node)
            {
                if (r.link)
                {
                    if (r.node->left && r.node->right)
                    {
                        // find left-most node, starting from right
                        Result rr = find_rotate_left_ (r.node->right);
                        
                        // correct parent link
                        if (rr.link)
                        { 
                            if (rr.node->right)
                                // leave right-tree behind
                                *(rr.link) = rr.node->right;
                            else 
                                // null terminate
                                *(rr.link) = 0;
                        }

                        // replace node
                        rr.node->left = r.node->left;
                        rr.node->right = r.node->right;
                        *(r.link) = rr.node;
                    } 
                    else if (r.node->left && !r.node->right)
                        *(r.link) = r.node->left;

                    else if (!r.node->left && r.node->right)
                        *(r.link) = r.node->right;

                    else
                        *(r.link) = 0;
                }
                else 
                    head_ = 0;
                        
                delete r.node;
                -- size_;
            }

            return n;
        }

        // note: apply in sorted order
        template <typename F> 
        void apply (F fn) 
        { 
            apply_inorder_ (head_, fn); 
        }

    private:
        Result find_rotate_left_ (Node *n)
        {
            Result r (0, n);

            for (;;)
            {
                if (!r.node->left)
                    break;

                r.link = &(r.node->left);
                r.node = r.node->left;
            }

            return r;
        }

        template <typename F>
        void apply_preorder_ (Node *n, F fn)
        {
            if (!n) return;
            fn (n->value);
            apply_preorder_ (n->left, fn);
            apply_preorder_ (n->right, fn);
        }

        template <typename F>
        void apply_inorder_ (Node *n, F fn)
        {
            if (!n) return;
            apply_inorder_ (n->left, fn);
            fn (n->value);
            apply_inorder_ (n->right, fn);
        }

        template <typename F>
        void apply_postorder_ (Node *n, F fn)
        {
            if (!n) return;
            apply_postorder_ (n->left, fn);
            apply_postorder_ (n->right, fn);
            fn (n->value);
        }

    private:
        size_t  size_;
        Node    *head_;
};

template <typename T>
void print (const T &v)
{
    cout << v << " ";
}

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    BinarySearchTree <int> tree;

    tree.insert (7);
    tree.insert (3);
    tree.insert (5);
    tree.insert (6);
    tree.insert (2);
    tree.insert (4);
    tree.insert (1);
    tree.insert (9);
    tree.insert (8);

    tree.erase (3);
    tree.apply (print<int>);

    return 0;
}
