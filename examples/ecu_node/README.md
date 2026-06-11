# ECU Node Example

Use this when you want to see the complete library surface working together:
identity, runtime DTCs, lifecycle state, explicit persistence, transport
callbacks, and capsule serialization.

This is the richest example and the most expensive profile. It is useful for
learning the whole integration model, but many products should start smaller and
enable only the modules they need.

## What To Notice

- Identity identifies the reporting node with compact numeric fields.
- DTC records use a fixed caller-owned array.
- Lifecycle records reset-related state.
- Storage is explicit and capsule-backed.
- Transport is attached as a callback interface, not as a protocol
  implementation.

Use this example as a reference when building a full diagnostic node. Use the
smaller examples when estimating footprint for a constrained product.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_TRANSPORT=ON \
  DIAG_FEATURE_CAPSULE=ON

./build/linux-debug/examples/diag_ecu_node_example
```

Expected output:

```text
ecu_node: confirmed DTC 0x050001, saved <n> bytes
```

After this works, run `./build.py size --dtc-capacity <count>` with a realistic
DTC capacity for your product.
