#!/bin/bash
find . -name '*.uc' > uc-files.txt
ctags -L uc-files.txt -f tags --options=unreal-ctags.config 
