#include <iostream>
#include <fstream>

std::ifstream::pos_type filesize (std::ifstream &file)
{
    std::ifstream::pos_type curr = file.tellg(); 
    file.seekg (0, std::ifstream::end);
    std::ifstream::pos_type last = file.tellg(); 
    file.seekg (curr);

    return last;
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

    std::cout << filesize (file) << std::endl;

    return 0;
}
