/* string.cpp --
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <map>
#include <set>
#include <list>

#include <tr1/cstdint>
#include <cctype>
#include <climits>
#include <cassert>

using namespace std;

int string_to_int (const char *str)
{
    bool leading = true;
    int value = 0;
    int sign = 1;

    if (str)
    {
        for (char c; *str; ++str)
        {
            if (*str == '-' && leading)
                sign = -1;

            if (*str == ' ' || *str == '\t')
                continue;
            else
                leading = false;

            c = *str - '0';

            if (c >= 0 && c < 10)
                value *= 10, value += c;
        }
    }

    return sign * value;
}

char most_frequent_char (char *buf)
{
    int count [26];
    int i, freq, ch;

    for (i=0; i < 26; ++i)
        count [i] = 0;

    for (char c; c = *buf; ++buf)
    {
        if (isalpha (c))
        {
            i = toupper (c) - 'A';
            assert (i >= 0 && i < 26);
            ++ count [i];
        }
    }

    ch = 26; freq = 0;

    for (i=0; i < 26; ++i)
    {
        if (count[i] > freq)
        {
            ch = i;
            freq = count[i];
        }
    }

    return ch + 'A';
}

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

list <string> permute_characters (string s, int begin, int end)
{
    list <string> res, sub;
    int size = end - begin;

    if (size == 1)
    {
        res.push_back (s);
    }
    else if (size >= 2)
    {
        for (int n = begin; n < end; ++n)
        {
            // fix one character
            swap (s[begin], s[n]);

            // recursively permute the rest
            sub = permute_characters (s, begin+1, end);

            // collect in the result
            res.splice (res.end(), sub, sub.begin(), sub.end());
        }
    }

    return res;
}

list <string> permute_characters (const string &s)
{
    return permute_characters (s, 0, s.size ());
}

void sort (list <string> &l, int pos = 0)
{
    if (l.size() < 2)
        return;
    else if (l.size() == 2)
    {
        if (l.front().at (pos) > l.back().at (pos))
            swap (l.front(), l.back());
        return;
    }

    vector <list <string> > bucket (26);

    list<string>::iterator i = l.begin();
    list<string>::iterator e = l.end();
    for (char c; i != e; ++i)
    {
        c = toupper (i->at (pos)) - 65;
        bucket[c].push_back (*i);
    }

    l.clear ();
    vector<list <string> >::iterator bi = bucket.begin();
    vector<list <string> >::iterator be = bucket.end();
    for (; bi != be; ++bi) 
    {
        sort (*bi, pos+1);
        l.splice (l.end(), *bi);
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

    list <string> l = permute_characters ("abcd");
    list<string>::iterator i = l.begin(); 
    list<string>::iterator e = l.end(); 
    for (; i != e; ++i) cout << *i << endl;

    l.clear ();
    l.push_back ("age");
    l.push_back ("aft");
    l.push_back ("art");
    l.push_back ("agog");
    l.push_back ("agag");
    l.push_back ("sage");
    l.push_back ("bob");
    sort (l);

    copy (l.begin(), l.end(), ostream_iterator <string> (cout, " "));
    cout << endl;

    cout << string_to_int("  1 234") << endl;
    cout << string_to_int("-1 234 ") << endl;

    return 0;
}
