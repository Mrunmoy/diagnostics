# Generic Diagnostics Library

A small C99 diagnostics core for embedded firmware that needs fault tracking,
reset history, device identity, and optional persistence without being tied to
CAN, ISO-TP, flash drivers, filesystems, or an RTOS.

This library is for products that eventually need to answer questions like:

- What fault happened, and is it still active?
- Is the fault pending, confirmed, or aging out?
- How many times did it occur or clear?
- Did the device reset abnormally?
- Which product, device type, and instance reported the problem?
- Which diagnostic state should survive a reset, and when is it safe to write it?

`libdiag` owns the diagnostic state machine. Your firmware owns the platform.
You decide how faults are detected, where bytes are stored, how messages are
framed, and when storage writes are allowed.

## Why This Exists

Many embedded projects start with a few fault flags and later grow into a mix of
runtime errors, persistent trouble codes, reset counters, service tools, and
bootloader/application handoff state. That usually becomes product-specific code
that is hard to reuse and easy to tie to one bus or storage device.

This project keeps those concerns separated:

- The core tracks DTC state, counters, operation-cycle behavior, lifecycle state,
  and compact identity.
- Storage and transport are callback interfaces supplied by the application.
- Persistence uses an explicit serialized capsule, not raw C structures.
- Runtime memory is caller-owned and fixed at initialization.
- Feature switches remove unused modules from constrained builds.

No heap. No hidden flash writes. No platform locks. No protocol assumption.

## Mental Model

Your application monitors real conditions. When something crosses a threshold,
it reports that fact to `libdiag`. The library updates bounded RAM state and
marks persistent sections dirty when needed. Later, at a time your product
chooses, you can save the dirty state through your storage adapter.

```mermaid
graph LR
    APP["Application monitors<br/>temperature, voltage, watchdog, sensors"]
    CTX["diag_context<br/>caller-owned RAM"]
    DTC["DTC module<br/>pending, confirmed, aging, counters"]
    LIFE["Lifecycle module<br/>reset reason and reset counters"]
    ID["Identity module<br/>compact numeric identity"]
    CAPS["Capsule<br/>versioned persistent byte format"]
    STORE["Your storage adapter<br/>flash, EEPROM, RAM, file"]
    TRANS["Your transport adapter<br/>CAN, UART, TCP, BLE, test harness"]
    TOOL["Your protocol/tooling<br/>UDS-like, binary, JSON, custom"]

    APP -->|"diag_dtc_set_active()<br/>diag_lifecycle_observe_reset()"| CTX
    CTX --> DTC
    CTX --> LIFE
    CTX --> ID
    DTC -->|"dirty flag"| CAPS
    LIFE -->|"dirty flag"| CAPS
    CAPS -->|"diag_save() / diag_load()"| STORE
    TOOL --> TRANS
    TRANS -.-> CTX
```

The important boundary is this: setting a fault never writes flash. Fault paths
update RAM. Persistence happens only when your firmware calls `diag_save()`.

## What You Provide

| You provide | Why |
|-------------|-----|
| Fault monitors | The library does not know your hardware limits or safety rules. |
| Caller-owned memory | Context storage, DTC arrays, and capsule buffers are supplied by you. |
| Operation-cycle timing | You decide what a meaningful cycle means for the product. |
| Storage callbacks | Flash, EEPROM, files, RAM, and wear-leveling policy are platform concerns. |
| Transport callbacks | CAN, UART, TCP, BLE, and test harnesses all fit behind the same shape. |
| Protocol/framing layer | The library is not a UDS server; it provides state for one if you build it. |

## What The Library Provides

| Library surface | What it gives you |
|-----------------|-------------------|
| `diag_context` | The opaque runtime home for enabled modules. |
| DTC module | Fixed-capacity trouble codes, status bits, counters, confirmation, aging. |
| Lifecycle module | Reset reason handling and reset counter policies without write-on-boot defaults. |
| Identity module | Numeric ecosystem/product/device identity for host-side catalogs. |
| Storage module | Explicit load/save/clear adapter contract. |
| Capsule module | Versioned, CRC-protected persistence format for DTC and lifecycle sections. |
| Transport module | Minimal send/receive adapter hook for your diagnostic protocol layer. |

## First Integration

Start with RAM-only DTCs. This proves the core model without storage, transport,
or persistence:

```c
#include <diag/diag.h>

static struct diag_context_storage context_storage;
static struct diag_dtc_snapshot    dtc_records[4];

struct diag_context *ctx = NULL;
struct diag_config config = {0};

diag_init(&context_storage, &config, &ctx);

struct diag_dtc_config dtc_config = {
    .records = dtc_records,
    .capacity = 4u,
};

diag_dtc_attach(ctx, &dtc_config);
diag_dtc_register(ctx, 0x010001u, DIAG_DTC_SEVERITY_ERROR);

if (sensor_reading_is_invalid())
{
    diag_dtc_set_active(ctx, 0x010001u);
}
else
{
    diag_dtc_set_inactive(ctx, 0x010001u);
}

diag_dtc_operation_cycle(ctx);
```

That is enough to get a bounded DTC record with UDS-style status behavior and no
storage writes. Add lifecycle, identity, storage, capsule, or transport only
when the product needs them.

## Choose A Starting Example

The examples are not just build samples; each one represents a product shape and
an embedded tradeoff. Start with the closest scenario:

| If your product needs... | Read this first | Why |
|--------------------------|-----------------|-----|
| A tiny link/lifetime check | `examples/basic` | Shows the smallest possible context integration. |
| A few runtime faults that reset on power cycle | `examples/sensor_node` | DTC + identity with no persistence cost. |
| Local DTC behavior only | `examples/dtc` | Focuses on registration, active state, counters, and operation cycles. |
| Reset reason/counter policy | `examples/lifecycle` | Shows reset tracking without forcing flash writes. |
| Numeric device identity only | `examples/identity` | Useful when host tooling owns names and catalogs. |
| Callback contracts | `examples/adapters` | Shows how storage and transport adapters are shaped. |
| Confirmed faults that survive restart | `examples/process_controller` | Adds capsule persistence for important confirmed state. |
| Critical thermal/reset diagnostics | `examples/industrial_oven` | Persists only important service data. |
| Separate bootloader and app diagnostics | `examples/bootloader_app_shared` | Uses separate capsule banks instead of shared raw structs. |
| A full embedded node profile | `examples/ecu_node` | Exercises identity, DTCs, lifecycle, storage, transport, and capsule. |

Each example README explains when to use that profile, what code to inspect, why
features are disabled, and what the output means.

## Quick Start

```sh
git clone https://github.com/Mrunmoy/diagnostics.git
cd diagnostics
git checkout c
./build.py all
```

If you prefer SSH, use `git@github.com:Mrunmoy/diagnostics.git`.

Useful commands:

```sh
./build.py build
./build.py test
./build.py test --preset linux-asan
./build.py all
./build.py feature-matrix
./build.py size --dtc-capacity 16 --write-alignment 16 --sections dtc,lifecycle
./build.py format --check
./build.py library
./build.py clean
```

Pass CMake options after `--`:

```sh
./build.py build -- DIAG_FEATURE_DTC=ON DIAG_FEATURE_STORAGE=OFF
```

`./build.py all` is the main local gate. It runs formatting checks, debug tests,
ASAN/UBSAN tests, release installation, package-consumption smoke tests, size
reporting, and the feature matrix.

## Docker And VS Code

Build and test in Docker:

```sh
docker compose build
docker compose run --rm diagnostics-dev
```

Inside the devcontainer, use:

```sh
./build.py all --preset container-debug
```

VS Code users can open the repository in the Dev Containers extension and use
the provided CMake, CTest, and debug configurations.

## Repository Layout

```text
.
├── cmake/                  # CMake helper modules
├── .devcontainer/          # VS Code Dev Container definition
├── .vscode/                # Build, test, and debug tasks
├── docs/design.md          # Single design source of truth
├── examples/               # Scenario-focused reference integrations
├── include/diag/           # Public C API
├── src/                    # Library implementation
├── tests/                  # GoogleTest behavior tests
├── tools/docker/           # Docker build/test image
├── CMakeLists.txt
├── CMakePresets.json
├── docker-compose.yml
└── README.md
```

## Downstream Use

Most embedded projects should consume this repository as a submodule and keep
their concrete adapters in the application tree:

```text
application
├── third_party/generic-diagnostics
├── platform/my_flash_storage.c
├── platform/my_transport.c
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

## Branch Strategy

- `main`: intentionally empty or documentation-only landing branch.
- `c`: embedded C99 implementation.
- `cpp`: reserved for the parallel C++17 implementation.

Public behavior should eventually exist on both implementation branches, each in
the idiom of that language.

## Going Deeper

- [docs/design.md](docs/design.md) explains the architecture, capsule format,
  storage contract, protocol boundary, bootloader/application sharing model, and
  resource policy.
- `include/diag/` is the public API surface.
- `tests/` captures exact behavior contracts.
- `examples/` shows product-shaped integration profiles.
