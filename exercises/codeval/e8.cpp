#include <iostream>
#include <fstream>
#include <sstream>

typedef unsigned int uint32_t;

// bit indices are 1-based
int sum_digits (char const *s)
{
    int result = 0;
    for (int i=0; s[i]; ++i)
        result += s[i] - '0';
    return result;
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "input must be a valid filename" << std::endl;
        return -1;
    }

    std::ifstream file (argv[1]);

    if (!file)
    {
        std::cout << "input must be a file" << std::endl;
        return -2;
    }

    size_t const N = 2048;
    char buffer [N];

    while (file.getline (buffer, N).eof() == false)
    {
        std::cout << sum_digits (buffer) << std::endl;
    }

    return 0;
}
