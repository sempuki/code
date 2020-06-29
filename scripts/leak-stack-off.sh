#!/usr/bin/env bash
bazel build common/framework:stack_run ${1/./_} && \
valgrind --leak-check=full --error-exitcode=1 --suppressions=$HOME/Work/av/build/valgrind_suppressions.txt \
./bazel-bin/common/framework/stack_run --logtostderr --logger --config_path $HOME/Work/av ${@}
