#!/bin/bash

basedir=$(pwd)
pushd "$1"; find . -type f | sort > "$basedir"/1.txt; popd
pushd "$2"; find . -type f | sort > "$basedir"/2.txt; popd
diff -u 1.txt 2.txt
rm -f 1.txt 2.txt
