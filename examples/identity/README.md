# Identity Example

Use this when a device needs to identify itself to a host tool but does not need
local DTC, reset, storage, or transport support from the library.

Identity is intentionally compact and numeric. Firmware should not carry long
product names, descriptions, or catalogs. A host-side tool can map ecosystem ID,
product ID, device type, device instance, and firmware component into human
meaning.

## What To Notice

- `struct diag_identity` contains fixed-width numeric fields.
- `diag_identity_attach()` copies the identity into the context.
- `diag_identity_get()` reads it back for a protocol layer or application code.

Choose this profile when the firmware only needs a stable identity block and a
separate protocol layer will expose it.

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

Success means the attached identity was read back exactly.
