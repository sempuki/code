#!/bin/bash
find . -name '*.java' >> gtags-files.txt
gtags -f gtags-files.txt
