# Design Specification

## Purpose

This library provides generic diagnostic services for embedded and host
applications. It is inspired by UDS diagnostic concepts, especially diagnostic
trouble code management, but it is not an implementation of ISO 14229 and does
not require CAN.

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

The first version focuses on:

- create/register code
- set active
- set inactive
- clear one code
- clear all codes
- retrieve one code
- enumerate codes
- increment/read/reset counters
- save/load state through storage abstraction

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

All public APIs return `diag_result_t`. Output parameters are only valid on
`DIAG_OK`.

## Memory Model

The core uses caller-provided configuration and storage. Dynamic allocation is
not allowed in the core implementation.

All capacities must be explicit:

- maximum DTC records
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
