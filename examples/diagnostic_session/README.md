# Diagnostic Session Example

This example shows the complete reason to use the library: firmware records
diagnostic state, and an external PC/tester reads and clears it without private
debug access.

## What Lives Where

- `device.c` simulates embedded firmware. It owns the `diag_context`, registers
  DTCs, records one failed monitor, saves persistent state through a fake storage
  adapter, and exposes a simulated device endpoint.
- `../common/example_diag_tool.c` contains the reusable PC diagnostic tool. It
  sends requests, validates responses, prints identity and DTC information,
  clears one DTC, and reports errors to `stderr`.
- `main.c` wires the simulated device and shared tester together in one process
  so CI, ASAN, and new users can run the whole workflow without hardware.

## Feature Set

This example uses:

- `DIAG_FEATURE_DTC=ON` for DTC registration, status, counters, clear, and list.
- `DIAG_FEATURE_IDENTITY=ON` for compact device identity.
- `DIAG_FEATURE_STORAGE=ON` and `DIAG_FEATURE_CAPSULE=ON` for explicit
  persistence after device-side state changes.

Lifecycle and transport are not required here. The request/response link is
implemented in `examples/common/` so the diagnostic core remains
transport-agnostic while multiple examples share the same tester behavior.

## Build And Run

From the repository root:

```sh
./build.py test -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Run only this executable after a build:

```sh
./build/linux-debug/examples/diag_diagnostic_session_example
```

Run it under the sanitizer preset:

```sh
./build.py test --preset linux-asan -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

## Expected Output

The exact capsule byte count may change as the serialized format evolves, but
the flow should look like this:

```text
diagnostic_session tool: opening diagnostic session
diagnostic_session tool: identity ecosystem=7 product=42 type=3 instance=1 stage=1 component=2
diagnostic_session tool: DTC count=2
diagnostic_session tool: DTC 0x030101 status=0x6d severity=2 occurrences=1
diagnostic_session tool: DTC 0x030102 status=0x40 severity=1 occurrences=0
diagnostic_session tool: cleared DTC 0x030101
diagnostic_session tool: DTC count=2
diagnostic_session tool: DTC 0x030101 status=0x00 severity=2 occurrences=1
diagnostic_session tool: DTC 0x030102 status=0x40 severity=1 occurrences=0
diagnostic_session tool: persisted capsule bytes=100
diagnostic_session tool: diagnostic session complete
```

## Extending This Pattern

A real PC tool can replace the in-process exchange in `examples/common/` with
serial, TCP, CAN, or another link. A GUI should sit above the tester-side model:
request identity, read DTCs, decode numeric IDs through a host catalog, clear
records, and show clear error messages when the device rejects a request.

Grafana can be useful when diagnostics are streamed or scraped into a time-series
backend. It is better for fleet dashboards and long-running observability than
for direct service actions such as clearing DTCs. A Grafana example should
therefore export tester-side results as metrics or logs rather than replacing
the diagnostic tester workflow.
