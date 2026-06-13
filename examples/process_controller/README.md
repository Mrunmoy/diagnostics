# Process Controller Example

Use this when a product has monitored process faults and only confirmed,
service-relevant state should survive a restart. This is the profile for systems
where one bad sample should not immediately become persistent diagnostic history.

The example enables DTC, lifecycle, identity, storage, and capsule. Transport is
disabled because the CLI tester uses the in-process example protocol.

## What To Notice

- The DTC confirmation threshold requires repeated failed operation cycles.
- Runtime DTC updates mark state dirty but do not call storage.
- `diag_save()` is called explicitly after the product decides persistence is
  appropriate.
- The fake storage adapter represents a platform-owned flash/EEPROM layer.
- The shared CLI tester reads identity, lists the confirmed DTC, clears it, and
  shows the persisted capsule size after the clear.

This profile is useful when flash endurance matters. You can report faults in
RAM frequently, then save only confirmed important state at a controlled point in
your application.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON \
  DIAG_FEATURE_TRANSPORT=OFF

./build/linux-debug/examples/diag_process_controller_example
```

Expected output:

```text
process_controller: persisted confirmed DTC using <n> bytes
process_controller tool: opening diagnostic session
process_controller tool: identity ecosystem=1 product=30 type=3 instance=1 stage=1 component=1
process_controller tool: DTC count=1
process_controller tool: DTC 0x030001 status=0x6d severity=2 occurrences=1
process_controller tool: cleared DTC 0x030001
process_controller tool: DTC count=1
process_controller tool: DTC 0x030001 status=0x00 severity=2 occurrences=1
process_controller tool: persisted capsule bytes=<n>
process_controller tool: diagnostic session complete
```

`<n>` is the serialized capsule size written through the example storage
adapter. Real firmware would size this buffer using `./build.py size`.
