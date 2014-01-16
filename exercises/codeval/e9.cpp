#include <iostream>
#include <fstream>
#include <cstdlib>

typedef unsigned int uint32_t;

// bit indices are 1-based
int fibonacci (int n)
{
    int fn = 0, fp = 1;

    while (n--)
        fn += fp, std::swap (fn, fp);

    return fn;
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

    std::string buffer;

    while (std::getline (file, buffer).eof() == false)
    {
        std::cout << fibonacci (std::atoi (buffer.c_str())) << std::endl;
    }

    return 0;
}
