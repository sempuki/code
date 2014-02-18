#!/bin/bash
find . -name '*.java' > java-files.txt
gtags -f java-files.txt
