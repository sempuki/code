#!/bin/bash
rm -f gtags-file
find . \
    -name '*.py' -or \
    -name '*.java' -or \
    -name '*.cpp' -or \
    -name '*.cxx' -or \
    -name '*.cc' -or \
    -name '*.c' -or \
    -name '*.hpp' -or \
    -name '*.hxx' -or \
    -name '*.hh' -or \
    -name '*.h' -or \
    -name '*.ipp' -or \
    -name '*.tpp' \
    > gtags-files.txt
export GTAGSFORCECPP=1 # interpret .h as C++
gtags -f gtags-files.txt