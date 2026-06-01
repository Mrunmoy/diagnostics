# Architecture

## Layers

```text
Application
    |
Protocol adapter
    |
Diagnostics service API
    |
Diagnostics core
    |
Platform abstraction interfaces
    |
User-provided transport/storage implementations
```

## Modules

- `diag_context`: lifecycle, configuration, dependency injection.
- `diag_dtc`: diagnostic trouble code model and operations.
- `diag_storage`: persistence abstraction and serialization boundary.
- `diag_transport`: byte transport abstraction for protocol adapters.

## Dependency Rule

Core modules may depend on public diagnostic types and platform abstraction
interfaces. They must not depend on concrete transports, filesystems, operating
systems, or test doubles.

## State Ownership

The caller owns the `diag_context_t` object and any backing memory referenced by
configuration. The library initializes and mutates that state only through
public APIs.

