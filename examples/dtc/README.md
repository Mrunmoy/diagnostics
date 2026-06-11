# Runtime DTC Example

Use this when you want to understand the DTC state machine without the noise of
identity, lifecycle, storage, or transport. It is the smallest example that
shows real diagnostic behavior.

DTCs are more than fault flags. A record can be active, pending, confirmed, and
later aged out by operation cycles. The library maintains the status byte and
counters while your firmware decides when a monitor passes or fails.

## What To Notice

- The DTC table is a fixed caller-owned array.
- Duplicate registration and capacity are controlled by the library.
- Active/inactive updates do not call storage.
- `diag_dtc_operation_cycle()` is the boundary where pending and confirmed state
  advances.

Use this profile as a first step before deciding which DTCs deserve persistence.

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

Success means one RAM-only DTC was registered, updated, and confirmed through an
operation cycle.
