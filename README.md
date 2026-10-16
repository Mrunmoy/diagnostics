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

This branch has the first C++ scaffold, identity slice, volatile DTC slice,
lifecycle/reset-counter slice, capsule serialization slice, and explicit storage
persistence slice:

- CMake package export as `diag::diag`.
- Docker and devcontainer build environment.
- `build.py` entry point for build, test, ASAN, format, install, and package smoke tests.
- A small RAII `diag::Context` backed by caller-owned storage.
- Strong diagnostic ID types and `diag::Result` error handling.
- Compact numeric `diag::Identity` records attached to a context.
- Fixed-capacity DTC records backed by caller-owned RAM.
- Reset lifecycle snapshots with disabled, RAM-only, abnormal-only, every-N, and
  platform-owned counter policies.
- Versioned diagnostic capsules with bounded section tables, fixed-size table
  entries, little-endian fields, CRC validation, and caller-owned payload buffers.
- Storage adapter callbacks for explicit save, load, and clear operations.

The next slices will add example tester workflows in idiomatic C++.

## Quick Start

Use Docker if your host does not have the pinned Clang tools:

```sh
docker compose build diagnostics-dev
DIAG_DOCKER_UID="$(id -u)" DIAG_DOCKER_GID="$(id -g)" docker compose run --rm diagnostics-dev sh
```

Inside the container:

```sh
./build.py all --preset container-debug
./build/container-debug/examples/diag_basic_example
./build/container-debug/examples/diag_capsule_example
./build/container-debug/examples/diag_dtc_example
./build/container-debug/examples/diag_identity_example
./build/container-debug/examples/diag_lifecycle_example
./build/container-debug/examples/diag_storage_example
```

On a host with `clang++-16` and `clang-format-14`:

```sh
./build.py all
./build.py library
```

The installed package is written to `build/install/diag`.

## Feature Selection

Builds default to the full library. Disable unused slices with CMake options
after `--`:

```sh
./build.py build -- DIAG_FEATURE_IDENTITY=OFF DIAG_FEATURE_STORAGE=OFF
./build.py feature-matrix --preset container-debug
```

Available switches:

- `DIAG_FEATURE_DTC`: fixed-capacity DTC registration and runtime state.
- `DIAG_FEATURE_LIFECYCLE`: reset counters and lifecycle snapshots.
- `DIAG_FEATURE_IDENTITY`: compact numeric device identity.
- `DIAG_FEATURE_STORAGE`: `diag::Context` integration with a storage adapter.
- `DIAG_FEATURE_CAPSULE`: `diag::Context` integration with the capsule format.

`diag::Context` persistence requires both storage and capsule support. The
standalone storage adapter and capsule helper APIs remain available from the
installed headers and static library; these switches decide whether `Context`
uses them for persistent diagnostic state.

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
diag::DtcRecord      dtcs[8]{};
diag::Config         config{dtcs, 8U};
diag::Context        diagnostics{storage, config};
diag::Identity       identity{diag::EcosystemId{7U}, diag::ProductId{90U}};

if (diagnostics.attachIdentity(identity) != diag::Result::Ok)
{
    // handle error
}

if (diagnostics.registerDtc(diag::DtcId{0x040101U}, diag::DtcSeverity::Critical) !=
    diag::Result::Ok)
{
    // handle error
}

diag::LifecycleConfig lifecycle{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};
if (diagnostics.attachLifecycle(lifecycle) != diag::Result::Ok)
{
    // handle error
}

if (diagnostics.observeReset(diag::ResetReason::Watchdog) != diag::Result::Ok)
{
    // handle error
}
```

Persistent diagnostics use a downstream storage adapter. The library validates
the callback table and writes only when `savePersistent()` is called. Always call
`loadPersistent()` after attaching storage to restore previously saved state before
calling `savePersistent()`:

```cpp
std::uint8_t capsule[256]{};
diag::Storage storageAdapter{
    diag::StorageOps{platformLoad, platformSave, platformClear},
    platformStorageUser,
    diag::StorageCapabilities{0xFFU, 8U},
    capsule,
    sizeof(capsule),
};

if (diagnostics.attachStorage(storageAdapter) != diag::Result::Ok)
{
    // handle error
}

// Restore previously persisted state. On first boot this returns Ok with nothing
// to load; on subsequent boots it restores DTC records and lifecycle counters.
diagnostics.loadPersistent();

if (diagnostics.savePersistent() != diag::Result::Ok)
{
    // handle error
}
```

All runtime memory is caller-owned. Library code must not allocate from the heap.
One `diag::ContextStorage` may back only **one live `diag::Context` at a time**;
destroy that context before reusing the storage for a new one.

## Development Rules

Read [docs/design.md](docs/design.md) before changing public behavior. Keep
implementation work on `task/<short-name>` branches targeting `cpp`. Run:

```sh
./build.py format
./build.py all --preset container-debug
```

before opening a pull request.
