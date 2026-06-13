# Grafana Reader Example

This example is the first complete PC-side diagnostics viewing path in the
repository. It starts with the same simulated embedded device used by the C
example, collects diagnostics through the tester-side request/response helper,
exports those diagnostics over HTTP, lets Prometheus scrape them, and opens a
prebuilt Grafana dashboard.

The target firmware does not contain HTTP, JSON, Prometheus, or Grafana. Those
parts live on the PC/tooling side.

## What Runs Where

| Part | File or service | Role |
|------|-----------------|------|
| Simulated firmware | `device.c` | Creates identity, DTC records, storage, and a persisted capsule. |
| Tester collector | `../common/example_diag_tool.c` | Reads identity and DTC state through the shared diagnostic frame path. |
| Export formatter | `exporter.c` | Converts the tester snapshot to Prometheus text or JSON. |
| Local CLI demo | `main.c` | Runs the simulated device and exporter once. |
| HTTP exporter | `service/metrics_server.py` | Serves `/metrics`, `/snapshot.json`, and `/healthz` on the PC side. |
| Prometheus | `prometheus/prometheus.yml` | Scrapes the exporter every 2 seconds. |
| Grafana | `grafana/` | Provisions the datasource and dashboard automatically. |

## 1. Build And Test The C Example

From the repository root:

```sh
./build.py test -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Run the CLI demo:

```sh
./build/linux-debug/examples/diag_grafana_reader_example
```

Expected signals:

```text
diag_tester_up 1
diag_dtc_active{dtc_id="0x040101"} 1
diag_dtc_confirmed{dtc_id="0x040101"} 1
diag_dtc_active{dtc_id="0x040102"} 0
diag_capsule_persisted_bytes 100
```

The same binary can print only one export format:

```sh
./build/linux-debug/examples/diag_grafana_reader_example --prometheus
./build/linux-debug/examples/diag_grafana_reader_example --json
```

## 2. Start The Grafana Stack

Grafana runs in Docker for this example. You do not need to install Grafana,
Prometheus, or any dashboard service on the host.

From the repository root:

```sh
docker compose -f examples/grafana_reader/docker-compose.yml up --build
```

Leave that terminal running. The stack starts three services:

- `exporter` at `http://localhost:9108`
- `prometheus` at `http://localhost:9090`
- `grafana` at `http://localhost:3000`

Those services are containers from `examples/grafana_reader/docker-compose.yml`.
The Grafana container uses the published `grafana/grafana` image. All host
ports bind to `127.0.0.1`, so the dashboard is local to your machine by default.

## 3. Check The Exporter Directly

In another terminal:

```sh
curl http://localhost:9108/healthz
curl http://localhost:9108/metrics
curl http://localhost:9108/snapshot.json
```

`/metrics` should include:

```text
diag_dtc_active{dtc_id="0x040101"} 1
diag_dtc_active{dtc_id="0x040102"} 0
```

## 4. Check Prometheus

Open:

```text
http://localhost:9090/targets
```

The `generic-diagnostics` target should be `UP`.

Then open the Prometheus graph page and query:

```text
diag_dtc_active
```

You should see two DTC time series:

- `0x040101` has value `1`
- `0x040102` has value `0`

## 5. View The Grafana Dashboard

Open:

```text
http://localhost:3000
```

Login is disabled for this local example. Go to:

```text
Dashboards -> Generic Diagnostics -> Generic Diagnostics - Grafana Reader
```

Or open the dashboard directly:

```text
http://localhost:3000/d/generic-diagnostics-grafana-reader/generic-diagnostics-grafana-reader
```

The dashboard should show:

- tester scrape health: `1`
- active DTC count: `1`
- confirmed DTC count: `1`
- DTC table with `0x040101` active and `0x040102` inactive
- DTC occurrence count for `0x040101`
- persisted capsule size: `100 bytes`
- compact device identity labels

Anonymous access uses Grafana's Viewer role. The example is intended for
inspection, not dashboard editing or service actions.

## 6. Stop The Stack

Press `Ctrl+C` in the compose terminal, then run:

```sh
docker compose -f examples/grafana_reader/docker-compose.yml down
```

## Troubleshooting

If Grafana has no data, check Prometheus first:

```sh
curl http://localhost:9108/metrics
```

Then browse to `http://localhost:9090/targets`.

If `/metrics` fails, rebuild the exporter image:

```sh
docker compose -f examples/grafana_reader/docker-compose.yml build --no-cache exporter
```

If port `3000`, `9090`, or `9108` is already in use, stop the conflicting local
service or edit the left side of the `ports` entries in `docker-compose.yml`.
