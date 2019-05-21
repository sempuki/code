#!/usr/bin/env bash
bazel run ${1} --run_under="lldb -b -o \"run --gtest_filter=${2}\" -k bt"
