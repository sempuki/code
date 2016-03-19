#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

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
        std::vector<int> set;
        bool scanning = true;
        bool begin = false;
        int n = 0;

        while (buffer.eof() == false)
        {
            buffer >> n;

            if (scanning)
                set.push_back (n);
            else
                if (std::binary_search (set.begin(), set.end(), n))
                {
                    if (begin) begin = false;
                    else std::cout << ',';

                    std::cout << n ;
                }

            if (buffer.peek() == ';')
                scanning = false,
                begin = true;

            buffer.ignore (1, ',');
        }

        std::cout << std::endl;
    }

    return 0;
}
