# IO Module Example

## Use Case

An IO expansion module needs device identity and a transport adapter, but it does
not persist diagnostics locally.

## Enabled Features

- `DIAG_FEATURE_IDENTITY=ON`
- `DIAG_FEATURE_TRANSPORT=ON`
- `DIAG_FEATURE_DTC=OFF`
- `DIAG_FEATURE_LIFECYCLE=OFF`
- `DIAG_FEATURE_STORAGE=OFF`
- `DIAG_FEATURE_CAPSULE=OFF`

## Why This Configuration

The module can be discovered and queried over a project transport while keeping
the firmware footprint small. Storage and DTC state are omitted entirely.

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

## Expected Output

```text
io_module: looped back 3 diagnostic bytes
```
