#!/bin/bash
export GTAGSFORCECPP=1
find . -name '*.cpp' -or -name '*.h' > cpp-files.txt
find . -name '*.uc' > uc-files.txt
gtags -f cpp-files.txt
ctags -L uc-files.txt -f tags --options=unreal-ctags.config 
