#include <iostream>
#include <vector>

std::vector<int> generate_primes (int n)
{
    std::vector<int> primes;

    if (n-- > 0) primes.push_back (2);
    if (n-- > 0) primes.push_back (3);
    if (n-- > 0) primes.push_back (5);
    if (n-- > 0) primes.push_back (7);
    if (n-- > 0) primes.push_back (11);
    if (n-- > 0) primes.push_back (13);
    if (n-- > 0) primes.push_back (17);
    if (n-- > 0) primes.push_back (19);
    if (n-- > 0) primes.push_back (23);
    if (n-- > 0) primes.push_back (29);

    for (int i = 31; n > 0; ++i)
    {
        bool factor = false;
        std::vector<int>::iterator 
            p = primes.begin(), 
              e = primes.end();

        for (; p != e && !factor; ++p)
            factor = (i % *p) == 0;

        if (!factor) --n, primes.push_back (i);
    }

    return primes;
}

int main (int argc, char **argv)
{
    std::vector<int> primes = generate_primes (1000);
    std::vector<int>::iterator p = primes.begin(), e = primes.end();

    int total = 0;
    for (; p != e; ++p) 
        total += *p;

    std::cout << total << std::endl;

    return 0;
}
