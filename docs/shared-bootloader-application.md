# Bootloader/Application Sharing

## Problem

In embedded systems, diagnostic information often spans both bootloader and
application firmware:

- the bootloader may detect update, flash, reset, rollback, or image validation
  failures.
- the application may detect runtime sensor, actuator, communication, or logic
  faults.
- a diagnostic tester may need to retrieve both sets through one external
  protocol.
- a bootloader and application may be updated independently.

The design must allow both firmware stages to share diagnostic state without
requiring identical binaries, identical transports, or identical build systems.

## Design Direction

The unique part of this library should be a small stable diagnostic capsule:

```text
shared storage region
┌────────────────────────────────────┐
│ diagnostic capsule header           │
│ schema version                      │
│ producer identity                   │
│ generation counter                  │
│ integrity check                     │
├────────────────────────────────────┤
│ bootloader diagnostic bank          │
├────────────────────────────────────┤
│ application diagnostic bank         │
├────────────────────────────────────┤
│ optional handoff records            │
└────────────────────────────────────┘
```

The capsule is not a C struct ABI. It is a serialized data contract owned by the
library.

## Diagnostic Banks

A bank is a logical owner of diagnostic records.

Initial banks:

- `bootloader`
- `application`
- `shared`

The bootloader can write bootloader records and read application records. The
application can write application records and read bootloader records. Shared
records are reserved for faults that cross the image boundary, such as repeated
reset loops or failed handoff validation.

## Code Namespace

Diagnostic IDs should include an owner/domain concept so bootloader and
application teams do not collide.

Suggested 32-bit layout:

```text
31..28 severity or class
27..24 owner
23..16 subsystem
15..00 code
```

This is a recommended convention, not a hard dependency. The core should accept
application-defined IDs, but documentation and examples should encourage a
stable namespace.

When diagnostics span multiple devices, this local DTC namespace should be
combined with device identity. A code is globally meaningful only when paired
with product, device, firmware stage, and subsystem identity.

## Handoff Records

Some events are not long-lived DTCs but are important during stage transition.
Examples:

- boot reason
- reset reason
- selected firmware slot
- image verification result
- rollback reason
- previous application exit marker

These should be represented as explicit handoff records rather than hidden
global state.

## Compatibility Rules

- Never persist raw C structs.
- Persist a versioned binary schema with explicit endianness.
- Include length fields so unknown future sections can be skipped.
- Include integrity protection, such as CRC32, in the storage format.
- Mutating shared storage should be atomic at the adapter level where possible.
- A newer application must be able to ignore older unknown bootloader fields.
- A newer bootloader must be able to preserve unknown application fields.

## API Implications

The first implementation can keep the API simple, but the design should leave
room for:

```c
diag_set_bank(ctx, DIAG_BANK_BOOTLOADER);
diag_set_bank(ctx, DIAG_BANK_APPLICATION);
diag_handoff_write(ctx, key, data, size);
diag_handoff_read(ctx, key, buffer, buffer_size, &bytes_read);
```

Those APIs should only be added once tests define the expected behavior.
