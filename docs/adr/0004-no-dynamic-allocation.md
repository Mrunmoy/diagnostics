# ADR 0004: No Dynamic Allocation in the Core

## Status

Accepted

## Context

The library targets embedded systems, including bootloaders and constrained
devices where heap allocation may be unavailable, unsafe, or hard to analyze.

## Decision

The diagnostic core will not call `malloc`, `calloc`, `realloc`, `free`, `new`,
or `delete`. All runtime memory is supplied by the caller during initialization
or through explicit API buffers.

## Consequences

- Memory use is predictable.
- Tests must cover capacity limits.
- APIs need explicit buffer and capacity parameters.
- Optional platform adapters may allocate internally, but the core must not
  require that behavior.

