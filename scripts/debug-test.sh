#!/usr/bin/env bash
bazel run ${1} --run_under="lldb-8 -b -o \"run --gtest_filter=${2}\" -k bt"
