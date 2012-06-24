#!/bin/bash
find . -name '*.uc' -or -name '*.ini' > uc-files.txt
ctags -L uc-files.txt -f tags --options=unreal-ctags.config 
