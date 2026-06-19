# Repository Guidelines

## Branch Model — Read First

This repository ships the same diagnostics library in two languages, one per
branch. Confirm the branch before writing code:

| Branch | Contents | Language and style |
| ------ | -------- | ------------------ |
| `main` | Intentionally empty | No implementation code |
| `c` | C implementation | C99, snake_case `diag_` symbols |
| `cpp` | C++ implementation | C++17, modern embedded C++ |

The `c` and `cpp` branches are parallel ports of one design, not a shared source
tree. Public behavior changes usually need a matching idiomatic change on the
sibling branch.

## Project Structure

Public APIs live in `include/diag/`. Core implementation files live in `src/`.
Unit tests live in `tests/`, examples in `examples/`, the design document in
`docs/design.md`, and Docker tooling in `tools/docker/`. VS Code devcontainer
configuration is under `.devcontainer/`.

Keep platform-specific storage and transport implementations out of the core.
Concrete adapters belong in downstream projects or under `examples/` when they
are generally useful.

## Design Source Of Truth

Read `docs/design.md` before changing APIs or behavior. It is the only tracked
design document. Do not add scratch, deep-dive, task-list, or temporary planning
markdown files.

The core direction is:

- Runtime events and volatile DTCs stay in RAM.
- Only important persistent DTCs and lifecycle records enter storage.
- Persistent writes are explicit or policy-driven, never hidden in hot paths.
- Reset counter persistence is RAM-only or wear-aware by policy.
- Bootloader and application use separate capsule banks.
- Host tooling owns strings, catalogs, descriptions, and rich product meaning.

## Build, Test, And Development Commands

Use `build.py` as the preferred entry point:

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

Docker/devcontainer users should use the `container-*` presets:

```sh
DIAG_DOCKER_RUN_AS_ROOT=1 docker compose run --rm diagnostics-dev
./build.py all --preset container-debug
```

## Formatting

Formatting is controlled by `.clang-format`:

- Allman braces.
- 4-space indentation, no tabs.
- 100-column limit.
- LLVM base style with right-aligned pointers.
- Consecutive declarations are aligned.
- `clang-format-14` is pinned for local, Docker, and CI consistency.

Run `./build.py format` before every commit. CI runs `./build.py format --check`.

## C++ Style

- C++17 only.
- No exceptions, no RTTI, and no heap allocation in library code.
- Use namespace `diag`.
- Types use `PascalCase`; functions use `camelCase`; private members use `m_`;
  constants use `kCamelCase`.
- Put `[[nodiscard]]` on bool and error-returning APIs.
- Prefer RAII, `constexpr`, `static_assert`, and compile-time validation.
- Use strong types for diagnostic IDs, product IDs, counters, and capacities.
- Return `diag::Result` or `diag::ResultValue<T>`; never throw across the
  library boundary.
- Use `auto` only where it improves readability for iterators, lambdas, and
  obvious factory results.

Class layout should be predictable: public aliases, constants, constructors,
deleted copy/move operations, public API, private helpers, and private members.

## Embedded Constraints

Both language branches target constrained embedded systems:

- No recursion or unbounded loops in core code.
- All runtime memory is caller-owned and capacity-limited.
- No hidden flash writes.
- No raw C or C++ struct persistence.
- Serialized storage uses fixed-width fields, schema versions, section lengths,
  reserved bytes, and integrity checks.
- Optimize by design first; add low-level branch hints only when the cold path is
  clear and measured.

## Testing

Tests use GoogleTest. `./build.py test` builds and runs the suite; CTest is only
the launcher. Test files are C++ (`.cpp`) and use `TEST(...)`, `EXPECT_*`, and
`ASSERT_*`.

Name tests by behavior, for example:

```cpp
TEST(DiagContext, InitializesInCallerOwnedStorage)
```

Prefer TDD: add a focused failing test, implement the smallest behavior, then
refactor. Important coverage areas include fixed-capacity limits, invalid
arguments, serialization compatibility, no-allocation behavior, and storage or
transport adapter contracts.

Also test resource behavior directly:

- Runtime-only DTCs must not call storage save.
- Persistent DTC mutation should mark dirty state without immediate flash write.
- Reset counter policy must not force one flash write per boot.
- Unsupported schema versions must fail deterministically.
- Storage parsers must reject oversized or untrusted lengths before iterating.

## Commit And Pull Request Guidelines

Work on `task/<short-name>` branches cut from `cpp`. Use short imperative commit
subjects, for example:

```text
Add fixed-capacity DTC registration
Document diagnostic capsule format
```

Pull requests should describe the change, explain embedded memory impact, list
tests run, and call out API or serialized-format compatibility changes. When a
change alters public behavior, note whether the C branch needs a matching port.

## Architecture Notes

The diagnostic core must remain transport-agnostic and storage-agnostic. CAN,
UART, TCP, flash, EEPROM, filesystem, and RTOS behavior must enter through
abstraction interfaces. Shared bootloader/application state must use a versioned
serialized capsule, never raw structs.
