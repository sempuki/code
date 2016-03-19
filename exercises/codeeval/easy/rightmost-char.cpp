#include <iostream>
#include <fstream>
#include <sstream>

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

    while (std::getline (file, line).eof() == false)
    {
        std::stringstream buffer (line);
        std::string str;
        char ch;

        std::getline (buffer, str, ',');
        buffer >> ch;

        std::string::reverse_iterator i = str.rbegin();
        std::string::reverse_iterator end = str.rend();
        for (; i != end && *i != ch; ++i);

        int index = (i != end)? std::distance (i, end) : 0;

        std::cout << index-1 << std::endl;
    }

    return 0;
}
