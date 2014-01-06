#include <iostream>
#include <fstream>
#include <cstring>
#include <cctype>

void reverse (char *s, int p, int r)
{
    while (p < r) std::swap (s[p++], s[r--]);
}

void reverse_words (char *string, int length)
{
    if (length <= 1) return;
 
    // reverse whole string
    reverse (string, 0, length-1);
     
    // re-reverse each word
    int word = 0, end = 0;
    while (word < length)
    {
        for (; string[word] && std::isspace (string[word]) != 0; ++word);
        end = word;

        for (; string[end] && std::isspace (string[end]) == 0; ++end);
        reverse (string, word, end-1);
        word = end;
    }
}

int main (int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "input must be a valid filename" << std::endl;
        return -1;
    }

    std::ifstream file (argv[1]);
    size_t const LINESIZE = 1024;
    char line [LINESIZE];

    if (!file)
    {
        std::cout << "input must be a file" << std::endl;
        return -2;
    }

    while (file.getline (line, LINESIZE).eof() == false)
    {
        size_t length = std::strlen (line);
        if (length == 0) continue;
        
        reverse_words (line, length);
        std::cout << line << std::endl;
    }

    return 0;
}
