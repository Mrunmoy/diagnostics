# Repository Guidelines

## Branch Model — READ THIS FIRST

This repository ships the **same diagnostics library in two languages**, one per
branch. **Before writing any code, confirm which branch you are on and apply the
matching style below. Never mix languages or styles across branches.**

| Branch | Contents | Language & style |
|--------|----------|------------------|
| `main` | **Intentionally empty.** | No code. Do not commit implementation here. Used only for shared docs/CI scaffolding if anything. |
| `c`    | The C implementation of the library. | **C99.** snake_case `diag_` symbols, Allman braces. |
| `cpp`  | The C++ implementation of the library. | **C++17.** Personal house style (see C++ section). |

Rules for agents:

- **Determine the branch first** (`git rev-parse --abbrev-ref HEAD`). If you are
  on `main`, do not add implementation code — ask which branch the work belongs
  on, or switch to `c` / `cpp`.
- The `c` and `cpp` branches are **parallel ports of one design**, not a shared
  tree. A change to public behavior generally needs to land on **both** branches,
  each in its own idiom.
- **Formatting is identical on both branches** (Allman braces, 4-space indent,
  100-column) and is enforced by `.clang-format`. Only the language and naming
  differ.

## Project Structure & Module Organization

This repository is an embedded-first diagnostics library. Public APIs live in
`include/diag/`. Core implementation files live in `src/`. Unit tests live in
`tests/`, examples in `examples/`, design notes in `docs/`, and Docker tooling
in `tools/docker/`. VS Code Dev Container and debug configuration are under
`.devcontainer/` and `.vscode/`.

Keep platform-specific storage and transport implementations out of the core.
Concrete adapters should usually live in downstream projects or under
`examples/` if they are generally useful.

## Build, Test, and Development Commands

Use `build.py` as the preferred entry point:

```sh
./build.py build              # configure and build with linux-debug
./build.py test               # build and run the GoogleTest suite
./build.py format             # apply clang-format
./build.py format --check     # verify formatting
./build.py clean              # remove one preset build directory
./build.py clean --all        # remove build/ and dist/
./build.py library --prefix dist/diag
```

Pass CMake options with `--option`, for example:

```sh
./build.py build --option DIAG_BUILD_EXAMPLES=OFF
```

Docker/devcontainer users should use the `container-debug` preset:

```sh
docker compose run --rm diagnostics-dev
```

## Coding Style & Naming Conventions

### Formatting (both branches)

Formatting is controlled by `.clang-format` and is the **same on the `c` and
`cpp` branches**:

- **Allman braces** — opening brace on its own line for functions, types, and
  control structures.
- **4-space indentation, no tabs.**
- **100-column** limit, LLVM base style, right-aligned pointers.
- Run `./build.py format` before every commit; CI runs `./build.py format --check`.

### Shared philosophy (both branches)

Both implementations target constrained embedded systems:

- **No dynamic allocation** in library code. All runtime memory is caller-owned
  and capacity-limited.
- **Compile-time over runtime** where the language allows it.
- **Error codes, not exceptions** — return `diag_result_t` (negative/zero/positive
  convention in C++); never throw across the library boundary.
- **Comments explain *why*, not *what*.** Use `// ── Section ──` separators to
  group related code.

### `c` branch — C99

- C99 only; must remain suitable for the most constrained targets.
- `diag_` prefix for public symbols (snake_case); uppercase `DIAG_` prefix for
  enum constants and macros.
- Public headers are C; they may be consumed from C++ (tests do this via
  `extern "C"`).

### `cpp` branch — C++17 (personal house style)

Follow the personal C++ standards from the global `~/.claude/CLAUDE.md`. In
summary:

- **C++17** baseline — no C++20 features (no concepts, coroutines, modules).
- **No exceptions, no RTTI** in library code; no dynamic allocation
  (`new`/`std::vector`/`std::string`/`std::map`) in firmware code. Exceptions are
  allowed only at C-API boundaries, wrapped to return error codes.
- **Naming:** `PascalCase` types, `camelCase` methods, `m_` private members,
  `kCamelCase` constants, `snake_case` namespaces, `SCREAMING_SNAKE_CASE` macros.
- **`[[nodiscard]]`** on all bool/error-returning functions.
- Prefer `constexpr`, `static_assert`, `if constexpr`; **RAII everywhere**;
  `std::atomic` with explicit memory ordering.
- `auto` only for iterators, `make_unique`/`make_shared`, structured bindings, and
  lambdas — otherwise spell out the type.
- Class layout: public aliases → `static_assert` → `static constexpr` → ctors/dtors
  → deleted copy/move → public API → protected virtuals → private helpers/members.

## Testing Guidelines

**GoogleTest is the test framework on both branches. Do not write hand-rolled C
test executables or a custom `main()` runner.** `./build.py test` builds the
suite and runs it (CTest is only the launcher — `gtest_discover_tests` registers
the GoogleTest binary).

- Test files are **C++ (`.cpp`)** and use `TEST(...)` / `EXPECT_*` / `ASSERT_*`.
- On the **`c` branch**, the library stays pure C99; test files include the C
  headers under `extern "C"` and link the C library. (Yes — C99 code is tested
  with GoogleTest exactly this way.)
- On the **`cpp` branch**, tests include the C++ headers directly.
- GoogleTest is pulled in via `FetchContent` (pinned tag) — no system install
  required, and it never ends up in the installed/embedded artifact.

Name tests behavior-focused via the suite/case pair, e.g.
`TEST(DiagContextInit, RejectsNullArguments)`.

Prefer TDD: add a focused failing test, implement the smallest behavior, then
refactor. Important coverage areas include fixed-capacity limits, null argument
handling, serialization compatibility, no-allocation behavior, and storage or
transport adapter contracts.

## Commit & Pull Request Guidelines

This checkout does not currently contain Git history, so no existing commit
style can be inferred. Use short imperative commit subjects, for example:

```text
Add fixed-capacity DTC registration
Document diagnostic capsule format
```

Commit implementation to the `c` or `cpp` branch that matches the language —
**never to `main`** (it stays empty). When a change alters public behavior, note
in the PR whether the sibling branch needs the matching port.

Pull requests should describe the change, explain embedded memory impact, list
tests run, and call out any API or serialized-format compatibility changes.

## Architecture Notes

The diagnostic core must remain transport-agnostic and storage-agnostic. CAN,
UART, TCP, flash, EEPROM, filesystem, and RTOS behavior must enter through
abstraction interfaces. Shared bootloader/application state must use a versioned
serialized capsule, never raw C structs.
