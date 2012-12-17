/* cards.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <queue>
#include <stack>

using namespace std;

int string_to_int (const char *str)
{
    bool leading = true;
    int value = 0;
    int sign = 1;

    if (str)
    {
        for (char c; *str; ++str)
        {
            if (*str == '-' && leading)
                sign = -1;

            if (*str == ' ' || *str == '\t')
                continue;
            else
                leading = false;

            c = *str - '0';

            if (c >= 0 && c < 10)
                value *= 10, value += c;
        }
    }

    return sign * value;
}

int do_reference_impl (int const N)
{
    queue<int> deck, input;
    int rounds = 0;

    for (int n = 0; n < N; ++n)
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

        ++rounds;
    } 
    while (deck != input);

    return rounds;
}

int do_optimized_impl (int const N)
{
    int A[N], B[N], C[N];

    int rounds = 0;
    bool equal = false;

    int *input = A;
    int *table = B;
    int *deck = C;

    for (int n = 0; n < N; ++n)
        input[n] = deck[n] = n;

    do 
    {
        int start = 0;
        int index = 0;
        int stride = 1;
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
        
        swap (deck, table);

        ++rounds;

        equal = true;
        for (int n = 0; n < N && equal; ++n)
            equal = (input[n] == deck[n]);
    } 
    while (!equal);

    return rounds;
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        cerr << argv[0] << " <SIZE_OF_DECK>" << endl;
        return -1;
    }

    int const N = string_to_int (argv[1]);

    //cout << "number of rounds: " << do_reference_impl (N) << endl;
    //cout << "===========================" << endl;

    cout << do_optimized_impl (N) << endl;

    return 0;
}
