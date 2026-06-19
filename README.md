# Generic Diagnostics C++ Library

This branch contains the C++17 implementation of a transport-agnostic diagnostics
library for embedded systems. It is inspired by the useful parts of UDS: stable
device identity, diagnostic trouble codes, lifecycle counters, persistent records,
and external tester workflows. It does not bake in CAN, serial, Modbus, flash,
EEPROM, filesystems, or RTOS behavior.

The library is meant to be linked into firmware. Firmware owns the memory,
chooses which features to compile, decides what should persist, and connects the
diagnostic core to its storage and transport adapters.

## Current Status

This branch has the first C++ scaffold:

- CMake package export as `diag::diag`.
- Docker and devcontainer build environment.
- `build.py` entry point for build, test, ASAN, format, install, and package smoke tests.
- A small RAII `diag::Context` backed by caller-owned storage.
- Strong diagnostic ID types and `diag::Result` error handling.

The next slices will add identity, DTC records, lifecycle counters, capsule
serialization, and example tester workflows in idiomatic C++.

## Quick Start

Use Docker if your host does not have the pinned Clang tools:

```sh
docker compose build diagnostics-dev
DIAG_DOCKER_RUN_AS_ROOT=1 docker compose run --rm diagnostics-dev
```

Inside the container:

```sh
./build.py all --preset container-debug
./build/container-debug/examples/diag_basic_example
```

On a host with `clang++-16` and `clang-format-14`:

```sh
./build.py all
./build.py library
```

The installed package is written to `build/install/diag`.

## Consuming The Library

After `./build.py library`, a downstream CMake project can use:

```cmake
find_package(diag CONFIG REQUIRED)
target_link_libraries(app PRIVATE diag::diag)
```

Example code:

```cpp
#include <diag/diag.hpp>

diag::ContextStorage storage{};
diag::Context        diagnostics{storage};
```

All runtime memory is caller-owned. Library code must not allocate from the heap.

## Development Rules

Read [docs/design.md](docs/design.md) before changing public behavior. Keep
implementation work on `task/<short-name>` branches targeting `cpp`. Run:

```sh
./build.py format
./build.py all --preset container-debug
```

before opening a pull request.
