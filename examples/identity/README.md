# Identity Example

## Use Case

A product needs compact, numeric identity for fleet tools or diagnostic sessions,
but does not need local DTC or persistence support.

## Enabled Features

- `DIAG_FEATURE_IDENTITY=ON`
- `DIAG_FEATURE_DTC=OFF`
- `DIAG_FEATURE_LIFECYCLE=OFF`
- `DIAG_FEATURE_STORAGE=OFF`
- `DIAG_FEATURE_TRANSPORT=OFF`
- `DIAG_FEATURE_CAPSULE=OFF`

## Why This Configuration

Identity is intentionally numeric and compact. Host tooling owns names,
descriptions, and product catalogs.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_identity_example
```

The program exits with status `0` after identity readback matches.
