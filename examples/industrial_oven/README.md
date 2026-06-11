# Industrial Oven Example

Use this for equipment where a critical thermal fault or abnormal reset must be
available to service personnel after power is cycled. The example is not about
logging every transient; it is about preserving the few facts that matter.

The profile enables DTC, lifecycle, identity, storage, and capsule. Transport is
disabled because many products expose diagnostics through their own protocol
layer or through a service tool that is outside this example.

## What To Notice

- Identity is attached so the saved data can be associated with a product and
  device instance.
- A critical DTC is registered and activated.
- Lifecycle observes an abnormal reset reason.
- Persistence is explicit: dirty state is serialized to a capsule only when the
  example calls `diag_save()`.

This is the shape to follow when persistent diagnostics are valuable but flash
wear still matters. Runtime state can change often; saved state should be
deliberate.

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

Expected output:

```text
industrial_oven: persisted DTC 0x020001 in <n> bytes
```

The example storage is RAM-backed. A real product would replace it with a
wear-aware flash or EEPROM adapter.
