#include <iostream>

typedef unsigned long long uint64_t;

int main (int argc, char **argv)
{
    std::string s;
    std::cin >> s;

    // the number of way to choose a starting cut point, and ending cut point
    // (where to make the substring by excluding heading or trailing characters)
    // in a string of size N is "N choose 2" -- so the solution is analytic and doesn't
    // require computation by simulation

    size_t N = s.size();
    uint64_t result = (N * (N + 1)) / 2;
    std::cout << result;

    return 0;
}
