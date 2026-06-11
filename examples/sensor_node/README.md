# Sensor Node Example

Use this for a small device that has a few local fault conditions but does not
need those faults to survive reset. Typical faults might be sensor open-circuit,
sensor out of range, or invalid calibration data.

The profile enables DTC and identity only. DTC records live in caller-owned RAM,
so reporting a fault has no storage latency and cannot wear flash. Identity gives
host tooling enough numeric information to know which product and instance
reported the runtime fault.

## What To Notice

- `dtc_records` fixes the maximum number of tracked faults at compile time.
- `diag_dtc_register()` declares the DTCs the product can report.
- `diag_dtc_set_active()` updates RAM state when a monitor fails.
- There is no storage adapter and no capsule buffer.

Choose this shape when losing DTC state on reset is acceptable and low footprint
matters more than post-reset service history.

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

Expected output:

```text
sensor_node: active runtime DTC 0x010001
```
