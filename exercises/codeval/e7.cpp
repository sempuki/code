#include <iostream>
#include <fstream>
#include <sstream>

typedef unsigned int uint32_t;

// bit indices are 1-based
void lower_string (char *s)
{
    for (int i=0; s[i]; ++i)
        s[i] += (s[i] >= 'A' && s[i] <= 'Z')? 
            0x20 : 0;
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
        lower_string (buffer);
        std::cout << buffer << std::endl;
    }

    return 0;
}
