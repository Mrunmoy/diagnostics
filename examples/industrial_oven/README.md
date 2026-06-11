# Industrial Oven Example

## Use Case

A temperature-control device reports runtime faults and persists only critical
events, such as an over-temperature trip or abnormal reset.

## Enabled Features

- `DIAG_FEATURE_DTC=ON`
- `DIAG_FEATURE_LIFECYCLE=ON`
- `DIAG_FEATURE_IDENTITY=ON`
- `DIAG_FEATURE_STORAGE=ON`
- `DIAG_FEATURE_CAPSULE=ON`
- `DIAG_FEATURE_TRANSPORT=OFF`

## Why This Configuration

DTC and lifecycle state are useful for service after a restart. Persistence is
explicit through `diag_save()`, so normal fault updates stay in RAM and do not
write flash in hot paths.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON \
  DIAG_FEATURE_TRANSPORT=OFF

./build/linux-debug/examples/diag_industrial_oven_example
```

## Expected Output

```text
industrial_oven: persisted DTC 0x020001 in <n> bytes
```
