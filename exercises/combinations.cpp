#include <iostream>
#include <algorithm>
#include <utility>
#include <vector>

using std::cout;
using std::endl;

template <typename BidiIterator>
bool permute_lexigraphical (BidiIterator begin, BidiIterator end)
{
    bool has_permutation = false;

    if (std::distance (begin, end) > 1)
    {
        BidiIterator next = end;
        BidiIterator prev = end;

        // Start at the first low-order pair
        std::advance (next, -2);
        std::advance (prev, -1);

        // Find first in-order pair to make lexigraphically larger
        for (; next != begin && !(*next < *prev); --prev, --next);

        has_permutation = *next < *prev;

        if (has_permutation)
        {
            BidiIterator swap = end;

            // Start with the first low-order element
            std::advance (swap, -1);

            // Find first item lexigraphically larger than next
            for (; has_permutation && !(*next < *swap); --swap);

            std::iter_swap (next, swap);
            std::reverse (prev, end);
        }
        else
        {
            std::reverse (begin, end);
        }
    }

    return has_permutation;
}

template <typename BidiIterator>
bool permute_reverse_lexigraphical (BidiIterator begin, BidiIterator end)
{
    bool has_permutation = false;

    if (std::distance (begin, end) > 1)
    {
        BidiIterator next = begin;
        BidiIterator prev = begin;

        // Start at the first low-order pair
        std::advance (next, 1);
        std::advance (prev, 0);

        // Find first in-order pair to make lexigraphically larger
        for (; next != end && !(*next < *prev); ++prev, ++next);

        has_permutation = *next < *prev;

        if (has_permutation)
        {
            BidiIterator swap = begin;

            // Start with the first low-order element
            std::advance (swap, 0);

            // Find first item lexigraphically larger than next
            for (; has_permutation && !(*next < *swap); ++swap);

            std::iter_swap (next, swap);
            std::reverse (begin, prev);
        }
        else
        {
            std::reverse (begin, end);
        }
    }

    return has_permutation;
}

int main (int argc, char **argv)
{
    std::vector<int> test = {1, 2, 3, 4};

    cout << "permute lexigraphical" << endl;

    do 
    {
        cout << "[";
        for (int i : test)
            cout << i << ",";
        cout << "]" << endl;
    }
    while (permute_lexigraphical (std::begin (test), std::end (test)));

    std::reverse (std::begin (test), std::end (test));

    cout << "permute reverse lexigraphical" << endl;

    do 
    {
        cout << "[";
        for (int i : test)
            cout << i << ",";
        cout << "]" << endl;
    }
    while (permute_reverse_lexigraphical (std::begin (test), std::end (test)));

    return 0;
}
