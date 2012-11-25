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

int main (int argc, char **argv)
{
    std::vector<int> test = {1, 2, 3, 4};

    do 
    {
        cout << "[";
        for (int i : test)
            cout << i << ",";
        cout << "]" << endl;
    }
    while (permute_lexigraphical (std::begin (test), std::end (test)));

    return 0;
}
