# Grafana Reader Example

This example shows how a PC-side diagnostic reader can turn device diagnostics
into observability data. The embedded device still uses compact numeric
diagnostics. The host-side reader collects identity and DTC state, then exports
Prometheus text and a JSON snapshot that a dashboard stack can consume.

Grafana is not the diagnostic protocol and it is not part of the target
firmware. It is a read-only view over data collected by a PC/tester.

## What Lives Where

- `device.c` simulates embedded firmware with DTC, identity, storage, and
  capsule persistence.
- `../common/example_diag_tool.c` acts as the PC/tester side and collects a
  host-side snapshot through the shared request/response path.
- `exporter.c` converts the tester snapshot into Prometheus text and JSON.
- `main.c` runs everything in one process so CI and ASAN can validate the full
  path without Grafana, Prometheus, sockets, or hardware.

## Build And Run

```sh
./build.py test -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON

./build/linux-debug/examples/diag_grafana_reader_example
```

Run with sanitizers:

```sh
./build.py test --preset linux-asan -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

## Expected Output

The output contains a Prometheus text block followed by JSON. Important stable
signals include:

```text
grafana_reader: prometheus metrics
diag_tester_up 1
diag_device_identity_info{ecosystem_id="7",product_id="90",device_type="4",instance="3",stage="1",component="1"} 1
diag_dtc_active{dtc_id="0x040101"} 1
diag_dtc_confirmed{dtc_id="0x040101"} 1
diag_capsule_persisted_bytes 100
grafana_reader: json snapshot
```

## Dashboard Boundary

A future optional Docker Compose profile may run Prometheus and Grafana against
an HTTP exporter. That should stay outside the default build and CI path. The
core library must not gain HTTP, JSON, Prometheus, Grafana, database, or
dashboard dependencies.

Use Grafana for read-only fleet and device health views: active fault counts,
confirmed DTCs, occurrence trends, reset/lifecycle trends, and tester scrape
health. Use a diagnostic tester or service tool for actions such as clearing
DTCs.
