# Runtime DTC Example

## Use Case

An embedded application needs bounded runtime fault tracking but does not need to
persist those faults after reset.

## Enabled Features

- `DIAG_FEATURE_DTC=ON`
- `DIAG_FEATURE_LIFECYCLE=OFF`
- `DIAG_FEATURE_IDENTITY=OFF`
- `DIAG_FEATURE_STORAGE=OFF`
- `DIAG_FEATURE_TRANSPORT=OFF`
- `DIAG_FEATURE_CAPSULE=OFF`

## Why This Configuration

The DTC buffer is caller-owned RAM. Registration, status updates, counters, and
operation-cycle confirmation work without any storage adapter, so there are no
hidden flash writes.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_dtc_example
```

The program exits with status `0` after confirming one RAM-only DTC.
