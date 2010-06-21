/* bst.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <stack>

using namespace std;

template <typename T>
struct BinaryNode
{
    T value;

    BinaryNode *left;
    BinaryNode *right;

    BinaryNode (const T &v) : 
        value (v), 
        left (0), 
        right (0) {}
};

template <typename T>
class BinarySearchTree
{
    public:
        typedef BinaryNode<T> node;
        typedef pair <node*, node*> result;

        BinarySearchTree () : size_ (0), head_ (0) {}
        ~BinarySearchTree () {}

        // note: first = parent, second = found
        result find (const T &v)
        {
            result r = make_pair <node *, node *> (0, head_);

            while (r.second)
            {
                if (v == r.second->value)
                    return r;
                
                else if (v < r.second->value)
                {
                    r.first = r.second;
                    r.second = r.second->left;
                }

                else 
                {
                    r.first = r.second;
                    r.second = r.second->right;
                }
            }

            return r;
        }

        node *insert (const T &v)
        {
            result r = find (v);
            node *n = 0;

            if (r.second)
                n = r.second;

            else
            {
                n = new node (v);
                ++ size_;

                if (r.first)
                {
                    if (v < r.first->value)
                        r.first->left = n; 
                    else
                        r.first->right = n;
                }
                else head_ = n;
            }
                
            return n;
        }

        node *erase (const T &v)
        {
            result r = find (v);
            node *n = 0;

            if (r.second)
            {
                delete r.second;
                -- size_;

                if (r.first)
                {
                    if (v < r.first->value)
                        r.first->left = 0;
                    else
                        r.first->right = 0;

                    n = r.first;
                }
                else head_ = 0;
            }

            return n;
        }

        // note: apply in sorted order
        template <typename F> 
        void apply (F fn) 
        { 
            apply_ (head_, fn); 
        }

    private:
        template <typename F>
        void apply_ (node *n, F fn)
        {
            if (!n) return;

            apply_ (n->left, fn);
            apply_ (n->right, fn);

            fn (n->value);
        }

    private:
        size_t  size_;
        node    *head_;
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

    tree.insert (5);
    tree.insert (3);
    tree.insert (6);
    tree.insert (1);

    tree.apply (print<int>);

    return 0;
}
