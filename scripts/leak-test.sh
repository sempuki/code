#!/usr/bin/env bash
bazel run ${1} --run_under="valgrind --leak-check=full --error-exitcode=1 --suppressions=$HOME/Work/av/build/valgrind_suppressions.txt"
