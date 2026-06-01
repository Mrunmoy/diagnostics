# ADR 0006: C API Uses Explicit Struct and Enum Tags

## Status

Accepted

## Context

The C branch targets embedded C users. In this environment, excessive typedefs
can hide useful information in API signatures and make code look more like C++
than C.

Typedefs are most valuable for platform abstraction, fixed-width types,
callback signatures, or semantic scalar IDs. They are less useful for ordinary
struct and enum names where the `struct` or `enum` keyword communicates useful
information.

## Decision

Public C APIs should use explicit struct and enum tags for objects and
configuration types.

Preferred:

```c
struct diag_context;
struct diag_config;

enum diag_result diag_init(struct diag_context_storage *storage,
                           const struct diag_config *config,
                           struct diag_context **out_ctx);
```

Avoid:

```c
diag_result_t diag_init(diag_context_storage_t *storage,
                        const diag_config_t *config,
                        diag_context_t **out_ctx);
```

Typedefs remain acceptable for:

- semantic scalar IDs, such as `diag_dtc_id_t`.
- callback function types when they improve readability.
- true platform portability aliases.

## Consequences

- Public APIs are more explicit and easier to read in embedded C code.
- The project avoids C++-style alias-heavy C.
- Existing scaffold headers should be refactored before the MVP API settles.
- C++ tests can still consume the C headers through `extern "C"`.

