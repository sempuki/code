#include <iostream>
#include <fstream>
#include <sstream>

typedef unsigned int uint32_t;

uint32_t first_one (uint32_t x)
{
    return 31u - __builtin_clz (x);
}

uint32_t nearest_multiple (uint32_t x, uint32_t n)
{
    uint32_t const mask = -1;
    uint32_t const width = sizeof(uint32_t) * 8;
    uint32_t const shift = first_one (n);

    uint32_t const low = x & (mask >> (width - shift));
    uint32_t const high = x & (mask << shift);

    return high + (low? n : 0);
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

    std::string line;
    uint32_t x, n;

    while (std::getline (file, line).eof() == false)
    {
        std::stringstream buffer (line);

        buffer >> x;
        buffer.ignore (1, ',');
        buffer >> n;

        std::cout << nearest_multiple (x, n) << std::endl;
    }

    return 0;
}
