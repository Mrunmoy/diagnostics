# Adapter Wiring Example

## Use Case

A downstream project wants to connect diagnostics to platform storage and a
project transport, but still owns the actual flash, EEPROM, UART, TCP, or test
harness implementation.

## Enabled Features

- `DIAG_FEATURE_STORAGE=ON`
- `DIAG_FEATURE_TRANSPORT=ON`
- `DIAG_FEATURE_DTC=OFF`
- `DIAG_FEATURE_LIFECYCLE=OFF`
- `DIAG_FEATURE_IDENTITY=OFF`
- `DIAG_FEATURE_CAPSULE=OFF`

## Why This Configuration

This example focuses only on callback contracts. It demonstrates that adapters
are plain function tables with an opaque `user` pointer.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_TRANSPORT=ON \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_adapters_example
```

The program exits with status `0` after attaching both adapters.
