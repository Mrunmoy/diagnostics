# IO Module Example

Use this when a small device needs to be discovered or queried over a project
transport but does not need local DTC storage. The point is to show that
transport is independent from diagnostics persistence.

The profile enables identity and transport only. DTC, lifecycle, storage, and
capsule are disabled, so the firmware avoids DTC arrays, persistence buffers,
and storage code.

## What To Notice

- Identity gives a protocol layer something useful to report.
- The transport adapter is just `send` and `receive` callbacks.
- The example loopback transport is deliberately simple; real framing belongs in
  your application protocol.

Choose this shape for small modules where another controller or host owns the
diagnostic history, and this firmware only needs to identify itself and exchange
diagnostic bytes.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_TRANSPORT=ON \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_io_module_example
```

Expected output:

```text
io_module: looped back 3 diagnostic bytes
```
