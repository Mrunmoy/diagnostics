# Examples

The examples are meant to answer a practical question: "Which parts of this
library should my firmware actually use?"

Each directory is a small product-shaped integration. Read the one closest to
your use case, then inspect its `main.c` to see the exact API calls.

| Example | Start here when... | Main lesson |
|---------|--------------------|-------------|
| `basic` | You only want to prove the library links and owns no heap. | Context lifetime with all optional modules disabled. |
| `sensor_node` | You need a few runtime faults and a compact identity. | RAM-only DTCs keep footprint low and avoid storage writes. |
| `dtc` | You want to understand DTC behavior before adding persistence. | Registration, active/inactive updates, counters, and operation cycles. |
| `lifecycle` | Reset reason or reset counters matter. | Reset tracking can stay RAM-only or request persistence by policy. |
| `identity` | A host tool needs to identify a device. | Firmware stores compact numeric IDs; host catalogs own strings. |
| `adapters` | You are ready to wire platform callbacks. | Storage and transport are small function tables with opaque user state. |
| `process_controller` | Only confirmed important faults should survive restart. | Confirmation thresholds and explicit capsule saves reduce flash churn. |
| `industrial_oven` | Critical thermal/reset state must be serviceable after restart. | Persist important DTC and lifecycle state, not every transient event. |
| `bootloader_app_shared` | Bootloader and application both report diagnostics. | Separate capsule banks avoid raw struct sharing and ownership fights. |
| `ecu_node` | You want the complete library surface in one place. | DTC, lifecycle, identity, storage, transport, and capsule working together. |

## How To Use These

1. Pick the closest scenario.
2. Read its README before reading `main.c`.
3. Build the feature profile shown there.
4. Replace the fake monitors, storage, or transport callbacks with platform code.
5. Run `./build.py size` with your planned DTC capacity and storage alignment.

Run every example enabled by a full-feature build:

```sh
./build.py test -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_LIFECYCLE=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_TRANSPORT=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Inspect feature and memory impact:

```sh
./build.py feature-matrix
./build.py size --dtc-capacity 8 --sections dtc,lifecycle
```
