# Basic Example

## Use Case

The smallest possible integration: initialize and deinitialize a diagnostics
context using caller-owned storage.

## Enabled Features

All optional features may be disabled.

## Why This Configuration

Use this when proving the library can be linked into a target before deciding
which diagnostic features the product needs.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_basic_example
```

The program exits with status `0` on success.
