// maintain a list of sorted values while supporting the following operations
// * find and remove the Nth value in the sequence

#include <cassert>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>

using std::cout;
using std::endl;

struct interval
{
    int begin, end;
    
    int size () const { return end - begin; }

    bool contains (int i) const 
    { 
        return size() > 0 && begin <= i && i <= end; 
    }
};

struct node
{
    node *left;
    node *right;

    int left_weight;
    int right_weight;
};

struct leaf : public node
{
    int value;
};

void init_node_recursive (node **current, int const **values, size_t const cardinality)
{
    if (cardinality == 0)
    {
        (*current) = nullptr;
    }
    if (cardinality == 1)
    {
        auto n = new leaf;

        n->value = **values;
        n->left = nullptr;
        n->right = nullptr;

        (*current) = n;
        (*values)++;
    }
    else
    {
        auto n = new node;

        auto left_cardinality = cardinality / 2;
        auto right_cardinality = cardinality - (cardinality / 2);

        n->left_weight = left_cardinality;
        n->right_weight = right_cardinality;

        init_node_recursive (&n->left, values, left_cardinality);
        init_node_recursive (&n->right, values, right_cardinality);

        (*current) = n;
    }
}

node *build_tree (std::vector<int> const &list)
{
    auto head = new node;
    size_t const size = list.size();
    int const *data = list.data();

    init_node_recursive (&head, &data, size);

    return head;
}

void print_leaf (node *n)
{
    cout << static_cast<leaf *> (n)->value;
}

void print_tree (node *n)
{
    if (n)
    {
        if (n->left && n->right)
        {
            cout << "[" << n->left_weight << "] | [" << n->right_weight << "]" << endl;
            print_tree (n->left);
            print_tree (n->right);
        }
        else
            print_leaf (n), cout << endl;
    }
}

void print_list (node *n)
{
    if (n)
    {
        if (n->left && n->right)
        {
            print_list (n->left);
            print_list (n->right);
        }
        else
            print_leaf (n), cout << ", ";
    }
}

int find_nth_recursive (node *n, interval i, int nth)
{
    if (n->left == nullptr && n->right == nullptr)
    {
        return static_cast<leaf *> (n)->value;
    }

    interval left = { i.begin + 0, i.begin + n->left_weight };
    interval right = { i.begin + n->left_weight, i.begin + n->left_weight + n->right_weight };

    if (left.contains (nth))
    {
        n->left_weight--;
        return find_nth_recursive (n->left, left, nth);
    }

    if (right.contains (nth))
    {
        n->right_weight--;
        return find_nth_recursive (n->right, right, nth);
    }

    assert (false && "assume input is valid");
}

int find_nth (node *n, int nth)
{
    return find_nth_recursive (n, {0, n->left_weight + n->right_weight}, nth);
}

int main ()
{
    cout << "hello world" << endl;

    std::vector<int> list = { 12, 23, 34, 45, 56, 67, 78, 90 };
    auto tree = build_tree (list);

    cout << find_nth (tree, 1) << endl;
    cout << find_nth (tree, 1) << endl;
    cout << find_nth (tree, 2) << endl;
    cout << find_nth (tree, 1) << endl;
    cout << find_nth (tree, 4) << endl;
    
    return 0;
}
