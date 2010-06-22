/* string.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <map>
#include <set>

#include <tr1/cstdint>
#include <cctype>

using namespace std;

string::const_iterator find_first_nonrepeating (const string &s)
{
    static const uint8_t SINGLE = 1;
    static const uint8_t REPEAT = 2;

    map <char, uint8_t> table;
    string::const_iterator i;

    for (i = s.begin(); i != s.end(); ++i)
    {
        if (table.count (*i))
        {
            if (table[*i] == SINGLE)
                table[*i] = REPEAT;
        }
        else
            table[*i] = SINGLE;
    }

    for (i = s.begin(); i != s.end(); ++i)
        if (table.count (*i) && table[*i] == SINGLE)
            break;

    return i;
}

void remove_characters (string &s, const string &chars)
{
    set <char> table (chars.begin(), chars.end());

    string::iterator in = s.begin();
    string::iterator out = s.begin();
    string::iterator end = s.end();

    while (in != end)
    {
        if (table.count (*in))
        {
            *in = 0;
            ++ in;
        }
        else
        {
            swap (*in, *out);
            ++ in; ++ out;
        }
    }
}

void reverse_words (string &s)
{
    string::iterator i = s.begin();
    string::iterator j = s.end();
    string::iterator e = s.end();

    // reverse string
    while (i < j) swap (*i++, *--j);

    // reverse words
    for (i = s.begin(); i < e; ++i)
    {
        for (j = i; isalpha (*j); ++j);
        while (i < j) swap (*i++, *--j);
        for (; isalpha (*i); ++i);
    }
}

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    string s1 ("the first non-repeating character is f");
    string s2 ("the quick fox jumped over the lazy brown dog.");
    string s3 ("Dilegua, o notte! Tramontate, stelle! Tramontate, stelle!"); 

    cout << *(find_first_nonrepeating (s1)) << endl;

    remove_characters (s2, "aeiou");
    cout << s2 << endl;

    reverse_words (s3);
    cout << s3 << endl;

    return 0;
}
