#include <iostream>
#include <fstream>
#include <cstdlib>

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

    int result = 0;
    int number = 0;

    while ((file >> number).eof() == false)
        result += number;

    std::cout << result << std::endl;

    return 0;
}
