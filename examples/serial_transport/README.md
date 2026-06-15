# Serial Transport Example

This example runs the same external diagnostic session over a POSIX serial port.
It is meant to look like a UART/USB-serial workflow while staying runnable on a
normal Linux machine and inside CI.

Two processes participate:

- `diag_serial_device_cpp` simulates embedded firmware. It owns the C
  diagnostics context, DTCs, identity, and persistent capsule.
- `diag_serial_tester_cpp` acts as the PC diagnostic tester. It uses the shared
  client from `examples/common/example_diag_client.cpp`.

The serial link carries one bounded diagnostic payload per byte-stream frame:

| Field | Size | Meaning |
|-------|------|---------|
| sync | 2 bytes | `0xA5 0x5A` frame marker |
| length | 2 bytes | little-endian payload size |
| payload | 0..160 bytes | `example_diag_frame` bytes |
| checksum | 1 byte | additive payload checksum |

This is deliberately small. A production UART transport can add sequence
counters, stronger CRC, retries, addressing, or segmentation below the same
diagnostic client.

## Build

From the repository root:

```sh
./build.py build -- \
  DIAG_BUILD_SERIAL_EXAMPLE=ON \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

## Run Without Hardware

The automated demo creates two pseudo-terminals and bridges them in Python:

```sh
python3 examples/serial_transport/run_pty_demo.py \
  --device build/linux-debug/examples/diag_serial_device_cpp \
  --tester build/linux-debug/examples/diag_serial_tester_cpp
```

CTest runs the same flow when the serial example is enabled:

```sh
./build.py test -- \
  DIAG_BUILD_SERIAL_EXAMPLE=ON \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Expected tester output includes:

```text
serial_tester: opening diagnostic session on /dev/pts/...
serial_tester: identity ecosystem=7 product=42 type=3 instance=1 stage=1 component=2
serial_tester: DTC count=2
serial_tester: DTC 0x030101 status=0x6d severity=2 occurrences=1
serial_tester: cleared DTC 0x030101
serial_tester: diagnostic session complete
```

## Run With Real Serial Ports

Use two connected ports, or a loopback adapter with a device process on one side
and the tester on the other:

```sh
./build/linux-debug/examples/diag_serial_device_cpp --port /dev/ttyUSB0
./build/linux-debug/examples/diag_serial_tester_cpp --port /dev/ttyUSB1
```
