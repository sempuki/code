"""Compiler flags for first-party targets: C++26 and warnings.

Kept in step with sempuki/lib's bazel/copts.bzl.
"""

_MSVC = [
    "/std:c++latest",
    "/Zc:__cplusplus",
    "/Zc:preprocessor",
    "/utf-8",
    "/permissive-",
    "/W4",
]

# C++26 is still spelled c++2c by some shipping compilers (Apple Clang).
_GNU = ["-std=c++2c", "-Wall", "-Wextra"]

COPTS = select({
    "@rules_cc//cc/compiler:msvc-cl": _MSVC,
    "@rules_cc//cc/compiler:clang-cl": _MSVC,
    "//conditions:default": _GNU,
})
