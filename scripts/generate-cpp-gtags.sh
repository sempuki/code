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
    -name '*.h' >> gtags-files.txt
gtags -f gtags-files.txt
