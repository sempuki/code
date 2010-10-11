/* main.cpp -- main module
 *
 *			Ryan McDougall
 */

#include <iostream>
#include <iomanip>

using namespace std;

// assumes BCD is big-endian
bool IncrementScore(unsigned char *scoreBuffer)
{
    unsigned char nibblemask = 0x0F;
    unsigned char nibbleshift = 0x00;
    unsigned char *begin = scoreBuffer;
    unsigned char *i = scoreBuffer+7;
    unsigned char c;

    while (i >= begin)
    {
        c = *i & nibblemask;
        c = c >> nibbleshift;
        c = (c + 1) % 10;

        *i = (*i & ~nibblemask) | c << nibbleshift;
        if (c) break;

        nibblemask ^= 0xFF;
        nibbleshift ^= 0x04;
        if (nibblemask == 0x0F) --i;
    }
}

// assumes BCD is big-endian
void PrintScore(unsigned char *scoreBuffer, ostream &out)
{
    unsigned char nibblemask = 0xF0;
    unsigned char nibbleshift = 0x04;
    unsigned char *i = scoreBuffer;
    unsigned char *end = scoreBuffer+8;
    unsigned char c;

    while (i < end)
    {
        c = *i & nibblemask;
        c = c >> nibbleshift;
       
        cout << (char) ('0' + c);

        nibblemask ^= 0xFF;
        nibbleshift ^= 0x04;
        if (nibblemask == 0xF0) ++i;
    }
}


//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    unsigned char score[8] = { 0x00, 0x00, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89 };
    PrintScore (score, cout); cout << endl;

    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);
    IncrementScore (score);

    PrintScore (score, cout); cout << endl;

    return 0;
}
