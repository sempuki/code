# code

- `course/`: C++ exercises that reimplement `std::function`, `std::unique_ptr`
  and `std::vector`. Each has a solution header and a Catch2 test suite.
- `exercises/`: old puzzle and experiment code, kept for reference and not
  built.
- `notes/`: design notes.

## Build

Requires [Bazelisk](https://github.com/bazelbuild/bazelisk) (installed as
`bazel`); the Bazel release is pinned in `.bazelversion`.

```sh
bazel test //...
```

Every course test suite builds twice. `test_<name>` runs against the solution
and `test_<name>_reference` runs against the standard library, which checks the
tests themselves.
