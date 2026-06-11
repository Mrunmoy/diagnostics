# Lifecycle Example

## Use Case

A device needs reset reason and reset counter policy, but persistence is handled
elsewhere or not required.

## Enabled Features

- `DIAG_FEATURE_LIFECYCLE=ON`
- `DIAG_FEATURE_DTC=OFF`
- `DIAG_FEATURE_IDENTITY=OFF`
- `DIAG_FEATURE_STORAGE=OFF`
- `DIAG_FEATURE_TRANSPORT=OFF`
- `DIAG_FEATURE_CAPSULE=OFF`

## Why This Configuration

This keeps reset tracking in RAM. The example uses abnormal-only policy so a
watchdog reset requests persistence, but no storage write happens unless a
storage feature and explicit save path are added.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_lifecycle_example
```

The program exits with status `0` after observing one abnormal reset.
