#include <iostream>
#include <cstdlib>

using namespace std;

const int N = 1000;

constexpr int maximum_multiple (int limit, int multiple)
{
    return (limit % multiple)? limit/multiple : (limit/multiple)-1;
}

constexpr bool is_even (int number)
{
    return !(number & 1);
}

constexpr int sum_to (int number)
{
    return is_even (number)? 
        (number+1) * (number/2) :
        (number+1) * (number/2) + (number/2)+1;
}

int main (int argc, char **argv)
{
    const int sum_of_threes = sum_to (maximum_multiple (N, 3));
    const int sum_of_fives = sum_to (maximum_multiple (N, 5));
    const int sum_of_threes_and_fives = sum_to (maximum_multiple (N, 3*5));

    cout << (3 * sum_of_threes) + (5 * sum_of_fives) - (3*5 * sum_of_threes_and_fives) 
        << endl;

    return 0;
}
