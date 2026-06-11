# Lifecycle Example

Use this when reset history matters but you are not ready to persist anything.
The lifecycle module records reset reasons and maintains reset counters according
to a policy selected by the application.

This is deliberately separate from DTC handling. A product may care about
watchdog resets even when it has no local trouble-code table, and a product may
want reset counting without writing flash on every boot.

## What To Notice

- The application reports the reset reason with `diag_lifecycle_observe_reset()`.
- The policy decides whether a reset only updates RAM or marks state dirty.
- With storage disabled, no save can happen; the example only shows lifecycle
  state transitions.

Use this profile when reset history is useful during runtime, or as the first
step before adding wear-aware persistence.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_lifecycle_example
```

Success means the example observed one abnormal reset and verified the lifecycle
state without writing storage.
