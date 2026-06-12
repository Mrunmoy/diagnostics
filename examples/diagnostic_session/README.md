# Diagnostic Session Example

This example shows the complete reason to use the library: firmware records
diagnostic state, and an external PC/tester reads and clears it without private
debug access.

## What Lives Where

- `device.c` simulates embedded firmware. It owns the `diag_context`, registers
  DTCs, records one failed monitor, saves persistent state through a fake storage
  adapter, and handles diagnostic requests.
- `tester.c` simulates the PC diagnostic tool. It sends requests, validates
  responses, prints identity and DTC information, clears one DTC, and reports
  errors to `stderr`.
- `protocol.h` defines the tiny example request/response protocol. It is local
  to this example; real products can put UDS, custom binary framing, UART, TCP,
  CAN, or another protocol in this layer.
- `main.c` wires the simulated device and tester together in one process so CI,
  ASAN, and new users can run the whole workflow without hardware.

## Feature Set

This example uses:

- `DIAG_FEATURE_DTC=ON` for DTC registration, status, counters, clear, and list.
- `DIAG_FEATURE_IDENTITY=ON` for compact device identity.
- `DIAG_FEATURE_STORAGE=ON` and `DIAG_FEATURE_CAPSULE=ON` for explicit
  persistence after device-side state changes.

Lifecycle and transport are not required here. The protocol link is deliberately
implemented in the example so the diagnostic core remains transport-agnostic.

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
tester: opening diagnostic session
tester: identity ecosystem=7 product=42 type=3 instance=1 stage=1 component=2
tester: DTC count=2
tester: DTC 0x030101 status=0x6d severity=2 occurrences=1
tester: DTC 0x030102 status=0x40 severity=1 occurrences=0
tester: cleared DTC 0x030101
tester: DTC count=2
tester: DTC 0x030101 status=0x00 severity=2 occurrences=1
tester: DTC 0x030102 status=0x40 severity=1 occurrences=0
tester: persisted capsule bytes=100
tester: diagnostic session complete
```

## Extending This Pattern

A real PC tool can replace `tester.c` transport calls with serial, TCP, CAN, or
another link. A GUI should sit above the tester-side model: request identity,
read DTCs, decode numeric IDs through a host catalog, clear records, and show
clear error messages when the device rejects a request.

Grafana can be useful when diagnostics are streamed or scraped into a time-series
backend. It is better for fleet dashboards and long-running observability than
for direct service actions such as clearing DTCs. A Grafana example should
therefore export tester-side results as metrics or logs rather than replacing
the diagnostic tester workflow.
