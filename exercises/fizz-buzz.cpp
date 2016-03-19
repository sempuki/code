#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

void fizzbuzz (int a, int b, int n)
{
    for (int i=1; i <= n; ++i)
    {
        bool fb = false;
        if (i % a == 0) fb = true, std::cout << 'F'; 
        if (i % b == 0) fb = true, std::cout << 'B';
        if (!fb) std::cout << i;
        std::cout << ' ';
    }

    std::cout << std::endl;
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "input must be a valid filename" << std::endl;
        return -1;
    }

    std::ifstream file (argv[1]);
    std::string line, a, b, n;

    if (!file)
    {
        std::cout << "input must be a file" << std::endl;
        return -2;
    }

    while (std::getline (file, line).eof() == false)
    {
        std::stringstream buffer (line);
        buffer >> a >> b >> n;
        fizzbuzz (std::atoi (a.c_str()), 
                  std::atoi (b.c_str()), 
                  std::atoi (n.c_str()));
    }

    return 0;
}
