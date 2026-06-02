# Diagnostics Library Design

## Purpose

This project builds an embedded-first diagnostics library inspired by UDS DTC
concepts, but not tied to CAN, ISO-TP, UART, TCP, any RTOS, or any storage
medium. The core provides diagnostic APIs; users provide transport, framing, and
storage adapters.

The design optimizes for constrained firmware:

- no heap allocation in core code.
- caller-owned memory and explicit capacities.
- no hidden flash writes.
- no raw C struct persistence.
- deterministic loops bounded by configured capacities.
- compact numeric records on target; rich strings and catalogs belong in host
  tools.

## Branch Strategy

- `main`: intentionally empty or docs-only.
- `c`: embedded C implementation.
- `cpp`: embedded C++ implementation.

Implementation must not be pushed directly to protected branches. Changes land
through PRs into `c` or `cpp`.

## C API Style

The C branch uses explicit `struct` and `enum` tags in public APIs. Do not
typedef ordinary structs/enums just to remove the keyword.

Preferred:

```c
struct diag_context;
enum diag_result diag_save(struct diag_context *ctx);
```

Typedefs are acceptable for semantic scalar IDs, callback signatures, or true
platform portability aliases.

Public objects that may evolve should be opaque. For embedded use, opaque
objects must still use caller-owned fixed storage rather than internal heap
allocation.

## Diagnostic Model

Diagnostics are separated by lifetime and storage cost:

- runtime event: no persistent state; useful for live reporting.
- volatile DTC: RAM state for the current boot; lost on reset.
- persistent DTC: stored in the diagnostic capsule; survives reset.
- lifecycle record: boot, reset, update, rollback, and handoff facts.

Only persistent DTCs and lifecycle records may dirty persistent storage.
Runtime and volatile updates must not write flash.

Suggested DTC status bits:

```text
bit 0 active_now
bit 1 pending
bit 2 confirmed
bit 3 seen_this_cycle
bit 4 seen_since_clear
bit 5 test_failed_last_cycle
bit 6 aged
bit 7 user/service
```

Counters should be saturating rather than wrapping. Repeated `set_active()` on
an already active DTC should be idempotent.

## Storage Capsule

Persistent state is stored as a versioned byte capsule, never as raw C structs.
The first schema should use a fixed header and bounded section table:

```text
header
section table
bootloader DTC bank
application DTC bank
shared lifecycle bank
handoff records
reset counters
reserved space
CRC / commit marker
```

The capsule includes:

- magic value.
- schema version.
- total length.
- generation counter.
- section count.
- per-section type, offset, length, used length, version, flags.
- explicit endian encoding.
- CRC/integrity check.

Unknown future sections should be skipped or preserved when safe. Unsupported
schema versions must fail deterministically rather than rewriting data that
cannot be understood.

Sizing targets:

```text
minimum capsule:      256 bytes
small default:        512 bytes
recommended product:  1024 bytes
persistent DTC:       about 16 bytes serialized
section count:        <= 8
```

## Bootloader And Application Sharing

Bootloader and application firmware may both report diagnostics. They may be
compiled differently and updated independently, so the persistent capsule is the
compatibility contract.

Use strict ownership banks:

- bootloader bank: bootloader writes, application reads.
- application bank: application writes, bootloader preserves.
- shared lifecycle bank: explicit policy only.
- handoff records: small transition facts.
- reset counters: policy-driven and wear-aware.

The bootloader should be conservative: read known fields, write only
bootloader-owned state, and preserve unknown application data. The application
can own richer migration, compaction, and full capsule rebuilds.

Reset counter persistence must not imply one flash write per boot by default.
Supported policies should include RAM-only, abnormal-reset-only, every-N resets,
platform-provided counters, and adapter-managed wear leveling.

## Platform Abstraction

The core depends on interfaces, not platforms.

Storage adapters own:

- erase blocks.
- write alignment.
- atomic commit.
- journaling or copy-on-write.
- wear leveling.
- power-fail recovery.

Transport adapters own:

- CAN, UART, TCP, BLE, SPI, test harnesses, etc.
- framing such as ISO-TP, SLIP, COBS, or length-prefix.
- partial reads/writes and nonblocking behavior.

The core and optional protocol handler operate on caller-owned buffers only.

## Protocol Boundary

The library owns diagnostic behavior. Protocol parsing and framing sit above or
beside it:

```text
transport -> framing -> optional protocol adapter -> diagnostics core
```

A future optional request/response handler may look like:

```c
enum diag_result diag_protocol_handle_request(
    struct diag_context *ctx,
    const uint8_t *request,
    size_t request_len,
    uint8_t *response,
    size_t response_cap,
    size_t *response_len);
```

The handler must not assume CAN, UDS negative response codes, sessions, security
access, or transport frame boundaries.

## Ecosystem Identity

A DTC ID alone is not globally unique across a product ecosystem. Host tooling
should construct a global diagnostic key from compact embedded facts:

```text
ecosystem id
product id
device type
device instance
firmware stage
subsystem
local DTC id
namespace/catalog version
```

Embedded firmware should store/report numeric IDs only. Host catalogs map those
IDs to names, descriptions, service procedures, firmware compatibility ranges,
and product-specific troubleshooting.

## Build And CI

Use `build.py` for local, Docker, and CI workflows:

```sh
./build.py all
./build.py test --preset linux-asan
./build.py build -- DIAG_BUILD_EXAMPLES=OFF
docker compose run --rm diagnostics-dev
```

`./build.py all` runs formatting, debug tests, ASAN/UBSAN tests, release library
install to `build/install/diag`, and a generated CMake package-consumption smoke
test. Generated artifacts must stay under `build/`.

CI runs on PRs and pushes targeting `c` and `cpp`. Protected branches require PR
review and passing CI before merge.

## TDD Expectations

Every feature starts with GoogleTest coverage. Tests are C++ files that include
C headers through `extern "C"` on the `c` branch.

Important test areas:

- null argument handling.
- fixed capacity limits.
- duplicate registration.
- no hidden storage writes for runtime/volatile DTCs.
- persistent mutations mark dirty state only.
- unsupported schema versions.
- malformed/oversized capsule input.
- storage/transport adapter contracts.
- ASAN/UBSAN clean host behavior.

