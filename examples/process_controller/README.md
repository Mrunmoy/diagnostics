# Process Controller Example

Use this when a product has monitored process faults and only confirmed,
service-relevant state should survive a restart. This is the profile for systems
where one bad sample should not immediately become persistent diagnostic history.

The example enables DTC, lifecycle, storage, and capsule. Identity and transport
are disabled to keep the focus on persistence policy.

## What To Notice

- The DTC confirmation threshold requires repeated failed operation cycles.
- Runtime DTC updates mark state dirty but do not call storage.
- `diag_save()` is called explicitly after the product decides persistence is
  appropriate.
- The fake storage adapter represents a platform-owned flash/EEPROM layer.

This profile is useful when flash endurance matters. You can report faults in
RAM frequently, then save only confirmed important state at a controlled point in
your application.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_TRANSPORT=OFF

./build/linux-debug/examples/diag_process_controller_example
```

Expected output:

```text
process_controller: persisted confirmed DTC using <n> bytes
```

`<n>` is the serialized capsule size written through the example storage
adapter. Real firmware would size this buffer using `./build.py size`.
