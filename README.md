# Generic Diagnostics Library

A transport-agnostic C diagnostics library inspired by UDS concepts, but not tied
to CAN, ISO-TP, or any specific bus.

The goal is to provide a small, portable diagnostic core that applications can
embed and connect to their own:

- transport layer: CAN, UART, TCP, BLE, SPI, test harness, etc.
- storage layer: RAM, flash, EEPROM, filesystem, database, etc.
- protocol framing: project-specific binary protocol, UDS-like protocol, JSON,
  or any other command format.

## Current Status

This repository is intentionally at the design-first stage. It contains:

- one design document
- public API skeletons
- platform abstraction interfaces
- Docker-based build and test environment
- TDD-oriented test layout

Implementation should be added by writing tests first, then filling in the
library behavior.

## Quick Start

Use the build wrapper for common workflows:

```sh
./build.py build
./build.py test
./build.py test --preset linux-asan
./build.py all
./build.py all --dtc-capacity 16 --write-alignment 16
./build.py feature-matrix
./build.py format --check
./build.py clean
```

`./build.py all` is the main local gate. It runs format checking, debug tests,
ASAN/UBSAN tests, release library installation, and a generated CMake package
consumption smoke test. Its size report accepts the same product-sizing options
as `./build.py size`. It also runs the feature matrix, which builds supported
feature profiles and checks disabled feature symbols are absent from `libdiag.a`.

Pass CMake cache options after `--`:

```sh
./build.py build -- DIAG_BUILD_EXAMPLES=OFF
./build.py all -- DIAG_BUILD_EXAMPLES=OFF
```

Build installable library output for another CMake project:

```sh
./build.py library
```

This produces headers, `libdiag.a`, and CMake package files under
`build/install/diag`.

Build and test in Docker:

```sh
docker compose build
docker compose run --rm diagnostics-dev
```

Open in VS Code Dev Containers:

1. Install the VS Code Dev Containers extension.
2. Run `Dev Containers: Reopen in Container`.
3. Use the CMake extension, or run the tasks:
   - `CMake: build container debug`
   - `CTest: container debug`
4. Use the debug configurations:
   - `Debug diagnostics tests`
   - `Debug basic example`

Or run CMake directly inside the container or on a Linux host:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

Inside the devcontainer, use the container preset:

```sh
./build.py all --preset container-debug
```

## Repository Layout

```text
.
├── cmake/                  # CMake helper modules
├── .devcontainer/          # VS Code Dev Container definition
├── .vscode/                # Build, test, and debug tasks
├── docs/                   # Single design document
├── examples/               # Example adapters and usage
├── include/diag/           # Public library API
├── src/                    # Library implementation
├── tests/                  # TDD tests
├── tools/docker/           # Docker build/test image
├── CMakeLists.txt
├── CMakePresets.json
├── docker-compose.yml
└── README.md
```

## Intended Downstream Use

Applications can consume this repository as a Git submodule and link `diag`.
Concrete storage and transport implementations should live in the consuming
project unless they are generic examples.

Example:

```text
application
├── third_party/generic-diagnostics
├── platform/my_flash_storage.c
├── platform/my_can_transport.c
└── app/diagnostic_protocol.c
```

As a submodule:

```cmake
add_subdirectory(third_party/generic-diagnostics)
target_link_libraries(app PRIVATE diag::diag)
```

As an installed package:

```cmake
find_package(diag CONFIG REQUIRED)
target_link_libraries(app PRIVATE diag::diag)
```

## Design Principles

- Diagnostic logic must not depend on CAN, sockets, filesystems, or RTOS APIs.
- All platform behavior is injected through small interfaces.
- Public APIs must be usable from embedded C projects.
- Dynamic allocation is not used by the core.
- All runtime capacity is caller-provided and fixed at initialization.
- Tests define behavior before implementation.
- Adapters are examples, not required dependencies.

## Branch Strategy

The intended long-term repository shape is:

- `main`: intentionally empty or documentation-only landing branch.
- `C`: embedded C implementation.
- `CPP`: embedded C++ implementation.

Both implementation branches should target constrained embedded systems. The C++
branch should not assume exceptions, RTTI, heap allocation, or the full standard
library unless explicitly enabled by configuration.
