# Design Specification

## Purpose

This library provides generic diagnostic services for embedded and host
applications. It is inspired by UDS diagnostic concepts, especially diagnostic
trouble code management, but it is not an implementation of ISO 14229 and does
not require CAN.

The aim is to keep embedded devices honest about diagnostic cost: no hidden
heap, no hidden flash churn, fixed capacities, and build-visible code, RAM, and
storage impact.

## Non-Goals

- No direct dependency on CAN, ISO-TP, SocketCAN, UART, TCP, or BLE.
- No mandatory filesystem, EEPROM, flash, or database dependency.
- No assumption that one diagnostic message equals one transport frame.
- No built-in security model in the first design phase.
- No global singleton state.
- No dynamic memory allocation in the core.

## Core Concepts

### Shared Diagnostic Domain

The diagnostic state may be shared between a bootloader and an application. The
library must therefore treat diagnostic data as a versioned contract, not as
private process memory.

The bootloader and application may be built at different times, with different
compiler settings, and may not use the same transport. Shared state must be
encoded through a stable serialization format and accessed through the same
public API contract.

### Ecosystem Diagnostic Domain

The same diagnostic tooling may need to inspect a product made of multiple
devices, boards, firmware images, or modules. A trouble code should therefore be
unique in context, not just unique inside one binary.

The design must support a stable identity model for:

- product family
- device type
- device instance
- firmware stage
- subsystem
- diagnostic trouble code

This lets one diagnostic tool distinguish the same local trouble-code value
reported by different devices in a larger ecosystem.

### Diagnostic Context

`diag_context_t` owns all runtime state for one diagnostic instance. Multiple
contexts may exist in one process if the user provides separate memory,
storage, and transport adapters.

### Diagnostic Trouble Code

A diagnostic trouble code is identified by an application-defined integer ID.
The core tracks status, severity, counters, and optional user metadata.

Not every DTC should persist. The design separates runtime-only diagnostics from
persistent diagnostics so flash is reserved for faults that matter after reboot.

The first version focuses on:

- create/register code
- register persistence policy
- set active
- set inactive
- clear one code
- clear all codes
- retrieve one code
- enumerate codes
- increment/read/reset counters
- save/load state through storage abstraction

### Diagnostic Persistence Classes

The core should support distinct persistence classes:

- runtime event: RAM only, no persistent state.
- volatile DTC: RAM state for the current boot, lost on reset.
- persistent DTC: stored in the diagnostic capsule, survives reset.
- critical lifecycle record: boot, reset, update, rollback, and handoff facts.

Only persistent DTCs and critical lifecycle records should cause non-volatile
storage writes.

### Platform Abstraction

The core depends on interfaces, not platforms:

- `diag_storage_ops_t` persists state.
- `diag_transport_ops_t` exchanges bytes with a caller-owned transport.
- future abstractions may cover clocks, locks, and allocation.

### Protocol Boundary

The library owns diagnostic behavior. Protocol parsing and framing belong above
or beside the core.

For example:

```text
CAN frame <-> ISO-TP <-> project command parser <-> diagnostics core
UART     <-> SLIP   <-> project command parser <-> diagnostics core
TCP      <-> bytes  <-> project command parser <-> diagnostics core
```

## Error Handling

All public APIs return a `diag_result` status code (`enum diag_result`):
`DIAG_OK == 0` on success and every other enumerator is a specific non-zero error.
Output parameters are only valid on `DIAG_OK`.

## C API Style

The C branch uses explicit `struct` and `enum` tags in public APIs instead of
typedef aliases for ordinary objects. This keeps embedded C signatures explicit
and avoids hiding object categories. This is the target style (see ADR 0006); the
current scaffold headers still expose `diag_result_t` / `diag_context_t` typedefs
and are being migrated to match.

Preferred:

```c
struct diag_context;
enum diag_result diag_save(struct diag_context *ctx);
```

Typedefs are reserved for semantic scalar IDs, callback signatures, and true
platform portability aliases.

## Memory Model

The core uses caller-provided configuration and storage. Dynamic allocation is
not allowed in the core implementation.

All capacities must be explicit:

- maximum DTC records
- maximum persistent DTC records
- maximum handoff records
- maximum serialized capsule size
- maximum transport payload handled by optional protocol helpers

Optional adapters may use dynamic allocation if their platform allows it, but
the core API and tests must not require it.

## Versioning

Public headers in `include/diag` define the API contract. Breaking API changes
must be documented in `docs/adr`.

Serialized diagnostic state must also be versioned. Bootloader/application
compatibility depends more on the persisted schema than on C struct layout.

## Size and Wear Goals

The MVP should support a minimum persistent capsule of 256 bytes, with 512 bytes
as a practical small default and 1 KB as a recommended product starting point.

Persistent reset counters are useful but write-sensitive. The reset counter
policy must be configurable so users can avoid writing flash on every boot.

Build tooling should eventually report library code size, static data, runtime
RAM cost, configured persistent storage size, and record capacities.
