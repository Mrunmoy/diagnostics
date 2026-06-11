# Bootloader/Application Shared Example

Use this when both a bootloader and an application need diagnostics, but they
must not share raw C structures or overwrite each other's persistent state.

The example uses two separate capsule banks: one for bootloader lifecycle/update
history and one for application DTC state. Both banks use the same serialized
format, but ownership is explicit.

## What To Notice

- The bootloader and application each initialize their own diagnostic context.
- The bootloader saves lifecycle information to its own storage bank.
- The application saves DTC information to a different storage bank.
- Nothing depends on the memory layout of `struct diag_context` or any private
  runtime structure.

This is the pattern to use when firmware stages need to exchange diagnostic
history through nonvolatile memory. The shared contract is the capsule format,
not a C header that exposes internal structs.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_TRANSPORT=OFF

./build/linux-debug/examples/diag_bootloader_app_shared_example
```

Expected output:

```text
bootloader_app_shared: bootloader bank <n> bytes, application bank <n> bytes
```

In real firmware, these banks would usually map to separate flash regions or
records with their own erase/write policy.
