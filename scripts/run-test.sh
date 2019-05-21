#!/usr/bin/env bash
bazel test ${1} || cat bazel-testlogs/${1/:/\/}/test.log
