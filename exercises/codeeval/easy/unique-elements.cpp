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
        int last = -1, n = 0;

        while (buffer.eof() == false)
        {
            buffer >> n;
            buffer.ignore (1, ',');
            
            if (n != last)
            {
                if (last != -1)
                    std::cout << ','; 

                std::cout << n;
                last = n; 
            }
        }

        std::cout << std::endl;
    }

    return 0;
}
