# ADR 0002: Use Caller-Owned Diagnostic Contexts

## Status

Accepted

## Context

Embedded users often need deterministic memory ownership and may not have a
heap.

## Decision

The caller owns `diag_context_t` and passes it to all public APIs. The initial
API does not require dynamic allocation.

## Consequences

- Multiple independent diagnostic instances are possible.
- The library is easier to test.
- Configuration must clearly define capacity limits.

