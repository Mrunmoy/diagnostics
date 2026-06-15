# Host Catalog Reader Example

This example shows why firmware should report compact numeric diagnostics and
leave human-readable meaning to the PC/tester side.

The simulated device exposes:

- compact identity fields: ecosystem, product, device type, stage, component.
- compact DTC records: DTC ID, status byte, severity, occurrence count.
- persisted capsule size.

The catalog reader acts like a diagnostic tester. It collects a snapshot from
the device, looks up the identity in a host-owned catalog table, and translates
each DTC into a name, meaning, and service action.

## Why Use This Shape

Use this pattern when target firmware must stay small and stable while host tools
need rich text. Firmware should not carry strings, translations, service manuals,
or product-specific troubleshooting text. A released catalog can be updated on
the PC side without changing the embedded diagnostic records.

## Build

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Run the example:

```sh
./build/linux-debug/examples/diag_catalog_reader_example
```

It is also part of `./build.py test` when the required features are enabled.

## Expected Output

The exact capsule byte count may change as the serialized format evolves, but
the output should show:

```text
catalog_reader: catalog=example-catalog-2026.06
catalog_reader: product=Reference Control Node role=bench diagnostic target instance=1
catalog_reader: compact identity ecosystem=7 product=42 type=3 stage=1 component=2
catalog_reader: DTC count=2 persisted_capsule_bytes=<n>
catalog_reader: DTC 0x030101 severity=error occurrences=1 status=0x6d active=yes pending=yes confirmed=yes failed_since_clear=yes
  name: Supply brownout detected
  meaning: The device observed a failed supply monitor during the current operation cycle.
  action: Check input supply range, connector retention, and brownout reset history.
```

## Files To Inspect

- `main.c`: host-side catalog lookup and presentation.
- `../diagnostic_session/device.c`: simulated embedded device that reports only
  compact identity and DTC records.
- `../common/example_diag_tool.c`: reusable tester-side snapshot collection.
