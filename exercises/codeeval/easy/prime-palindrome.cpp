#include <iostream>
#include <sstream>
#include <cmath>

// see: http://en.wikipedia.org/wiki/Primality_test#Naive_methods
//
// The algorithm can be improved further by observing that all primes 
// are of the form 6k ± 1, with the exception of 2 and 3.

bool is_prime (int n)
{
    const int limit = (std::sqrt (n) - 1) / 6;
    bool factor = (n % 2 == 0) || (n % 3 == 0);

    for (int i = 1; !factor && i <= limit; ++i)
        factor = 
            (n % ((6 * i) - 1)) == 0 || 
            (n % ((6 * i) + 1)) == 0;

    return factor == false;
}

bool is_palindrome (int n)
{
    std::stringstream ss; ss << n;
    std::string const &s = ss.str();
    size_t const N = s.size();

    for (int i=0; i < N/2; ++i)
        if (s[i] != s[N-i-1])
            return false;
    return true;
}

int main (int argc, char **argv)
{
    bool found = false;
    for (int n = 1000; n >= 31 && !found; --n)
        if (is_prime (n) && is_palindrome (n))
            found = true, std::cout << n << std::endl;

    return 0;
}
