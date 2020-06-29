#!/usr/bin/env bash
bazel build common/framework:stack_run ${1/./_} && \
./bazel-bin/common/framework/stack_run --logtostderr --logger --offline ${@}
