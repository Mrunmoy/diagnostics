# Repository Strategy

## Branches

The repository is intended to use separate implementation branches:

- `main`: intentionally empty or documentation-only landing branch.
- `C`: embedded C implementation.
- `CPP`: embedded C++ implementation.

This keeps the C and C++ APIs independent while allowing both to share the same
design principles and diagnostic model.

## C Branch

The C branch should prioritize:

- C99 or later, with embedded compiler compatibility.
- no heap allocation in the core.
- caller-owned memory.
- explicit capacity configuration.
- small ABI surface.
- easy use as a Git submodule.

## CPP Branch

The C++ branch should still be embedded-first:

- no required heap allocation.
- no required exceptions.
- no required RTTI.
- no hidden global objects.
- fixed-capacity containers or caller-provided buffers.
- RAII where it improves ownership clarity without runtime cost.

## Main Branch

The main branch should not become the default implementation branch. Its job is
to explain the project and point users to the correct implementation branch.

## Submodule Use

Downstream projects should be able to include either implementation branch as a
submodule and provide concrete platform adapters:

```text
firmware
├── third_party/diagnostics
├── board/storage_adapter.c
├── board/transport_adapter.c
└── app/diagnostics_protocol.c
```

