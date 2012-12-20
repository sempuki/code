/* cards.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <queue>
#include <stack>

using namespace std;

// To ensure I understood the problem correctly, I translated the wording
// into code as literally as I was able, which required the use of higher-
// level primitives to prototype with. I then compared the output of the
// reference implementation with hand computations on paper, and fixed one
// or the other as necessary until I was convinced that both my understanding
// and the reference implementation was correct.
//
// I using my intuition compared against the reference results, I mapped out
// the minimum required data and computation needed to solve the problem as
// requested. I then implemented an optimized solution, comparing the results
// it created against the reference implementation to ensure correctness.
//
// When the solution was completed after testing, I decided to leave the 
// reference implementation in place as a form of documentation of the 
// standard I used for correctness (in case the reference itself was 
// incorrect).

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
    int A[N], B[N];

    int rounds = 0;
    bool equal = false;

    int *table = A;
    int *deck = B;

    // initialize deck into sequential order

    for (int n = 0; n < N; ++n)
        deck[n] = n;

    do 
    {
        // until our current deck is in sequential

        int start = 0;      // first card in the deck
        int index = 0;      // index into the deck
        int stride = 1;     // stride to the next card
        int next = N-1;     // next card on the table
        bool take = true;   // take the card from deck to table

        // until our table is full

        while (next >= 0)
        {
            // starting from the first available card in the deck
            // (depending on whether the last card was taken into the table)

            index = start;
            start += take * stride;

            // put cards in the table when take == true
            // skipping all cards already in the table

            while (index < N)
            {
                table[next] = deck[index];

                index += stride;
                next -= take;
                take = !take;
            }

            // expand the stride to skip cards in the table

            stride <<= 1;
        }
        
        swap (deck, table);

        ++rounds;

        // we have returned when the cards are in sequential order again

        equal = true;
        for (int n = 0; n < N && equal; ++n)
            equal = (deck[n] == n);
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

    //cout << do_reference_impl (N) << endl;
    //cout << "=====" << endl;
    cout << do_optimized_impl (N) << endl;

    return 0;
}
