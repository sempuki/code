#!/usr/bin/env bash
bazel build common/framework:stack_run ${1/./_} && \
gdb -ex='set follow-fork-mode child' -ex='run' --args ./bazel-bin/common/framework/stack_run --logtostderr --logger --config_path $HOME/Work/av ${@}
