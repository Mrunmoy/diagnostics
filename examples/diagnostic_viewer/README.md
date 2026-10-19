# Diagnostic Viewer Example

This example shows the PC-side workflow that most users expect from a diagnostics
library: connect to a device, read its identity, see what is wrong, and request a
DTC clear from outside the firmware.

The embedded side is still plain C. It owns fixed diagnostic storage, registers a
small DTC catalog, updates fault state, and returns bounded responses through the
same tester helper used by the other examples. The GUI runs on the PC side and
polls a local HTTP server that executes the simulated device binary.

## Build

From the repository root:

```sh
./build.py test -- \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Run the device snapshot once:

```sh
./build/linux-debug/examples/diag_diagnostic_viewer_device --json --scenario-step 3
```

The JSON contains compact identity, DTC state, active/confirmed counts, and the
stored capsule size.

## Start The GUI

```sh
python3 examples/diagnostic_viewer/viewer_server.py
```

Open:

```text
http://127.0.0.1:8088
```

The page refreshes once per second. The simulated scenario moves through several
fault phases so the viewer does not look static:

- heater over-temperature starts active and confirmed.
- fan speed degradation appears, confirms, then clears.
- fieldbus heartbeat loss appears briefly as an informational DTC.
- stored capsule bytes stay visible so users see persistence cost.

Use the `Clear` buttons to exercise the PC-to-device service action. In a real
product, the HTTP server would call a transport adapter instead of launching the
simulated binary.

## Files

| File | Purpose |
|------|---------|
| `device.c` / `device.h` | Simulated embedded device and DTC scenario. |
| `main.c` | PC/tester entry point that prints JSON snapshots. |
| `viewer_server.py` | Local PC-side HTTP server for the browser GUI. |
| `static/` | Browser UI assets. |
