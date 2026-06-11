# Bootloader/Application Shared Example

## Use Case

A bootloader and an application both report diagnostics, but each owns a separate
persistent capsule bank. This avoids raw struct sharing and avoids one side
overwriting the other's state.

## Enabled Features

- `DIAG_FEATURE_DTC=ON`
- `DIAG_FEATURE_LIFECYCLE=ON`
- `DIAG_FEATURE_STORAGE=ON`
- `DIAG_FEATURE_CAPSULE=ON`
- `DIAG_FEATURE_IDENTITY=OFF`
- `DIAG_FEATURE_TRANSPORT=OFF`

## Why This Configuration

The bootloader needs lifecycle/update history. The application needs DTC state.
Both serialize through the same capsule format, but each uses a separate storage
bank. No raw context or C structure is persisted.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_TRANSPORT=OFF

./build/linux-debug/examples/diag_bootloader_app_shared_example
```

## Expected Output

```text
bootloader_app_shared: bootloader bank <n> bytes, application bank <n> bytes
```
