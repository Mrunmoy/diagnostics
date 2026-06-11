# Adapter Wiring Example

Use this when you are ready to connect the generic core to platform code. The
library does not know how to read flash, erase EEPROM, send UART bytes, or write
to a test harness. It only knows how to call the function tables you attach.

This example keeps DTC, lifecycle, identity, and capsule disabled so the callback
contracts are easy to see.

## What To Notice

- Storage is a table of `load`, `save`, and `clear` callbacks plus an opaque
  `user` pointer.
- Transport is a table of `send` and `receive` callbacks plus an opaque `user`
  pointer.
- The application owns the backing memory, driver state, locks, timing, and wear
  policy.

Use this as a template when writing a RAM fake for tests or a real adapter in a
downstream firmware project.

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

Success means both adapter tables were accepted and called through the generic
interfaces.
