# Repository Guidelines

## Branch Model

This branch contains the C++17 implementation of the diagnostics library. The
`c` branch contains the C implementation, and `main` intentionally stays empty.
Do not mix C implementation style into this branch.

## Project Structure

Public C++ headers live in `include/diag/`. Implementation files live in `src/`.
GoogleTest tests live in `tests/`, examples in `examples/`, design notes in
`docs/`, and Docker tooling in `tools/docker/`.

## Build And Test Commands

Use `build.py` as the main entry point:

```sh
./build.py build
./build.py test
./build.py test --preset linux-asan
./build.py all
./build.py format
./build.py format --check
./build.py library
./build.py clean --all
```

Pass CMake options after `--`, for example:

```sh
./build.py build -- DIAG_BUILD_EXAMPLES=OFF
```

Host and Docker presets use `clang-16` / `clang++-16`. Formatting is pinned to
`clang-format-14`.

## C++ Style

- C++17 only.
- Allman braces, 4-space indentation, 100-column limit.
- No exceptions, no RTTI, and no heap allocation in library code.
- Use `diag` namespace, `PascalCase` types, `camelCase` functions, `m_` private
  members, and `kCamelCase` constants.
- Use strong types for diagnostic IDs and counters instead of raw integers.
- Return `diag::Result` or `diag::ResultValue<T>`; do not throw.
- Prefer RAII and compile-time capacities.

## Testing

Tests use GoogleTest and CTest. Keep tests behavior-focused and avoid duplicate
coverage. Important areas are fixed-capacity behavior, no-allocation paths,
null/invalid construction handling, and package-consumption smoke tests.
