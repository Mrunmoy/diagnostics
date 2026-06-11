# ECU Node Example

## Use Case

A full embedded control unit needs identity, runtime DTCs, reset tracking,
explicit persistence, and a project transport hook.

## Enabled Features

- `DIAG_FEATURE_DTC=ON`
- `DIAG_FEATURE_LIFECYCLE=ON`
- `DIAG_FEATURE_IDENTITY=ON`
- `DIAG_FEATURE_STORAGE=ON`
- `DIAG_FEATURE_TRANSPORT=ON`
- `DIAG_FEATURE_CAPSULE=ON`

## Why This Configuration

This is the complete integration profile. It costs the most code space but gives
the downstream product every library surface: identity, DTC lifecycle, reset
policy, explicit capsule persistence, and transport callbacks.

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

## Expected Output

```text
ecu_node: confirmed DTC 0x050001, saved <n> bytes
```
