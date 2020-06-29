#!/usr/bin/env bash
sudo sh -c 'echo -1 > /proc/sys/kernel/perf_event_paranoid'
sudo sh -c 'echo 0 > /proc/sys/kernel/kptr_restrict'
bazel build common/framework:stack_run ${1/./_} && \
perf record -g ./bazel-bin/common/framework/stack_run --logtostderr --logger --offline ${@}
