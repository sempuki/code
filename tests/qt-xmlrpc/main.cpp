/* main.cpp -- main module
 *
 */

#include "main.h"
#include <QCoreApplication>

//=============================================================================
// Main entry point
int
main (int argc, char** argv)
{
    QCoreApplication app (argc, argv);
    TestApp test;

    return app.exec();
}
