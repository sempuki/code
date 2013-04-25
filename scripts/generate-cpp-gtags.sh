#!/bin/bash
export GTAGSFORCECPP=1
find . \
    -name '*.cpp' -or \
    -name '*.cxx' -or \
    -name '*.cc' -or \
    -name '*.c' -or \
    -name '*.hpp' -or \
    -name '*.hxx' -or \
    -name '*.hh' -or \
    -name '*.h' > cpp-files.txt
gtags -f cpp-files.txt
