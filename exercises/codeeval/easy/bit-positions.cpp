#include <iostream>
#include <fstream>
#include <sstream>

typedef unsigned int uint32_t;

// bit indices are 1-based
bool bit_same (uint32_t x, uint32_t i, uint32_t j)
{
    bool a = x & (1 << (i - 1));
    bool b = x & (1 << (j - 1));

    return !(a ^ b);
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
    uint32_t x, i, j;

    std::cout << std::boolalpha;
    while (std::getline (file, line).eof() == false)
    {
        std::stringstream buffer (line);

        buffer >> x;
        buffer.ignore (1, ',');
        buffer >> i;
        buffer.ignore (1, ',');
        buffer >> j;

        std::cout << bit_same (x, i, j) << std::endl;
    }

    return 0;
}
