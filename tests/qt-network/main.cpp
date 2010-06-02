/* main.cpp -- main module
 *
 *			Ryan McDougall -- 2009
 */

#include "main.h"

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    Test test (argc, argv);
    return test.Go();
}
