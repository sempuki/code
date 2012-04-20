#!/bin/bash
export GTAGSFORCECPP=1
find . -name '*.cpp' -or -name '*.h' > cpp-files.txt
gtags -f cpp-files.txt
