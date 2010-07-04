
#include <iostream>
#include <vector>

#include <cctype>
#include <cassert>

using namespace std;

vector <int> find_primes (int max)
{
    bool isprime;
    vector <int> primes;
    vector <int>::iterator p, e;

    primes.push_back (2);

    for (int i = 3; i < max; ++i)
    {
        isprime = true;

        for (p = primes.begin(), e = primes.end(); p != e; ++p)
        {
            if (i % *p == 0) 
            {
                isprime = false;
                break;
            }
        }
        
        if (isprime)
            primes.push_back (i);
    }
    
    return primes;
}

int main ()
{
    vector <int> p = find_primes (100000);
    return 0;
}
