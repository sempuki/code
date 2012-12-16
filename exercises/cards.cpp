#include <iostream>
#include <queue>
#include <stack>

using namespace std;

void do_reference_impl (int const N)
{
    queue<int> deck, input;

    for (int n=1; n <= N; ++n)
        input.push(n);
    deck = input;

    do 
    {
        int current;
        bool to_table = true;
        stack<int> table;

        while (!deck.empty())
        {
            current = deck.front(), deck.pop();

            if (to_table)
            {
                table.push(current);
            }
            else
            {
                deck.push(current);
            }

            to_table = !to_table;
        }

        while (!table.empty())
        {
            current = table.top(), table.pop(); 
            deck.push(current); 
            cout << current << ", ";
        }
        cout << endl;
    } 
    while (deck != input);
}

void do_optimized_impl (int const N)
{
    vector<int> input, deck, table;

    for (int n = 1; n <= N; ++n)
        input.push_back(n);
    deck = input;
    table.resize(N);

    do 
    {
        int start = 0;
        int stride = 1;
        int index = 0;
        int next = N-1;
        bool take = true;

        while (next >= 0)
        {
            index = start;
            start += take * stride;

            while (index < N)
            {
                table[next] = deck[index];

                index += stride;
                next -= take;
                take = !take;
            }

            stride <<= 1;
        }

        copy (begin (table), end (table), begin (deck));

        for (int v : deck)
            cout << v << ", ";
        cout << endl;

        ++count;
    } 
    while (deck != input);
}

int main (int argc, char **argv)
{
    int const N = 14;

    do_reference_impl (N);

    cout << "===========================" << endl;

    do_optimized_impl (N);

    return 0;
}
