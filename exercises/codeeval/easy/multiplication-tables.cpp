#include <iostream>
#include <cstring>

typedef unsigned int uint32_t;

int decimal_places (int n)
{
    int result = 1;
    while (n /= 10) result++;
    return result;
}

void pretty_print (std::ostream &out, int n)
{
    int M = 4;
    int N = M - decimal_places (n);
    int i = 0;

    char padding[M];
    while (i < N)
        padding[i++] = ' ';
    padding[i] = '\0';

    out << padding << n;
}

int matrix [12][12];

int main (int argc, char **argv)
{
    for (int j=0; j < 12; ++j)
        matrix[0][j] = j+1;

    for (int i=1; i < 12; ++i)
        for (int j=0; j < 12; ++j)
            matrix[i][j] = matrix[i-1][j] + j+1;

    for (int i=0; i < 12; ++i)
    {
        for (int j=0; j < 12; ++j)
            pretty_print (std::cout, matrix[i][j]);
        std::cout << std::endl;
    }

    return 0;
}
