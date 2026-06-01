# ADR 0003: Use a Versioned Diagnostic Capsule for Shared State

## Status

Accepted

## Context

The diagnostic system may be shared by bootloader and application firmware. They
may be compiled separately, updated independently, and run with different
available platform services.

## Decision

Shared diagnostic state will be represented as a versioned serialized capsule,
not as raw C structs. The capsule may contain separate banks for bootloader,
application, and shared records.

## Consequences

- Bootloader/application compatibility can be tested through binary fixtures.
- Stored diagnostic data can survive independent firmware updates.
- The storage abstraction remains generic.
- The implementation must include schema validation before mutating live state.

