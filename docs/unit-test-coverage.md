# Unit-test coverage

Run the C++ unit tests with coverage instrumentation:

```sh
make coverage
```

The command uses an isolated build under `build/coverage`. Each invocation
creates a separate `run.*` directory containing:

- `report.txt` — per-file terminal summary
- `html/index.html` — annotated, browsable source report
- `coverage.json` — machine-readable coverage data

The path of each generated report is printed when the command finishes.
Set `JSTINE_COVERAGE_DIR` to place the build and reports elsewhere.

## Toolchains

The script selects the reporting backend from the compiler detected by CMake:

- Clang and AppleClang use `llvm-profdata` and `llvm-cov`. On macOS, the
  script finds them through `xcrun`; on Linux it supports both unversioned
  tools and names such as `llvm-cov-18`.
- GCC uses `gcovr`, which must be installed and available on `PATH`.

Conan and CMake are required on both platforms. The coverage report includes
all objects from `jstined_lib`, including production files not linked into
the unit-test executable; untested files therefore appear as 0% rather than
being omitted.
