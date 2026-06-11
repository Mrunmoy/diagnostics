# Sensor Node Example

## Use Case

A small sensor device needs a compact identity and a few runtime fault records.
Faults are useful while the device is powered, but they do not need to survive a
reset.

## Enabled Features

- `DIAG_FEATURE_DTC=ON`
- `DIAG_FEATURE_IDENTITY=ON`
- `DIAG_FEATURE_LIFECYCLE=OFF`
- `DIAG_FEATURE_STORAGE=OFF`
- `DIAG_FEATURE_TRANSPORT=OFF`
- `DIAG_FEATURE_CAPSULE=OFF`

## Why This Configuration

This is a low-footprint profile. DTC records stay in caller-owned RAM and no
storage adapter is attached, so there are no flash writes and no persistence
buffer to allocate.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_sensor_node_example
```

## Expected Output

```text
sensor_node: active runtime DTC 0x010001
```
