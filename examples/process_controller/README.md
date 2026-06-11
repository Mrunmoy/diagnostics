# Process Controller Example

## Use Case

A process-control device tracks monitored faults and persists confirmed critical
state for post-restart diagnostics.

## Enabled Features

- `DIAG_FEATURE_DTC=ON`
- `DIAG_FEATURE_LIFECYCLE=ON`
- `DIAG_FEATURE_STORAGE=ON`
- `DIAG_FEATURE_CAPSULE=ON`
- `DIAG_FEATURE_IDENTITY=OFF`
- `DIAG_FEATURE_TRANSPORT=OFF`

## Why This Configuration

The controller needs persistence and reset policy, but it may not need an
identity block or built-in transport adapter. DTC confirmation uses two failed
operation cycles to avoid saving one-sample transients.

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

## Expected Output

```text
process_controller: persisted confirmed DTC using <n> bytes
```
