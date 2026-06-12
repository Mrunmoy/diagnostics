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
| `diagnostic_session` | You want to see the whole external diagnostics workflow. | A simulated device and PC/tester exchange requests to read identity, list DTCs, clear a DTC, and persist the result. |
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

## Device And Tester Sides

Onboarding examples must show both halves of the workflow. The firmware side
records diagnostic state through `diag_*` APIs. The PC/tester side sends
requests, decodes responses, prints useful output, and reports failures clearly.

`diagnostic_session` is the reference pattern for that shape. Smaller examples
remain useful when you only need to inspect one module or one feature switch.

## Future PC Tooling Examples

GUI examples must be built as real host tools, not quick visual wrappers. The
preferred Qt path is a C++17 Qt Widgets tester that calls the C API/protocol
boundary through a client layer. It should use MVC-style models, controllers,
small widgets composed into panels and tabs, Qt signal/slot boundaries, and a
worker thread for blocking transport work. Qt must stay optional behind a
separate build switch so the embedded library and default CI remain lean.

Grafana is useful as a read-only observability view over diagnostics collected
by a PC/tester. A Grafana-oriented example should export bounded Prometheus
metrics or JSON snapshots from the tester side. It must not suggest that target
firmware needs HTTP, JSON, Prometheus, Grafana, a database, or service actions
such as clearing DTCs through a dashboard.
