# Modbus RTU Transport Example

This example runs the external diagnostic session over a Modbus RTU-style link.
It uses the same shared diagnostic client as the SocketCAN and serial examples,
but wraps the request/response payload in a Modbus RTU application data unit.

Two processes participate:

- `diag_modbus_rtu_device_cpp` simulates a Modbus slave device.
- `diag_modbus_rtu_tester_cpp` simulates a PC diagnostic tester acting as the
  Modbus master.

The example uses a user-defined Modbus function code for diagnostics:

| Field | Size | Meaning |
|-------|------|---------|
| slave address | 1 byte | default `0x11` |
| function | 1 byte | `0x41`, diagnostic payload |
| length | 2 bytes | little-endian diagnostic payload size |
| payload | 0..160 bytes | shared `example_diag_frame` bytes |
| CRC16 | 2 bytes | standard Modbus RTU CRC, low byte first |

This keeps Modbus-specific framing below the shared diagnostic workflow. A real
product could replace the custom function with a register map or vendor-defined
function while keeping the same diagnostic service payload.

## Build

From the repository root:

```sh
./build.py build -- \
  DIAG_BUILD_MODBUS_RTU_EXAMPLE=ON \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

## Run Without Hardware

The automated demo creates two pseudo-terminals and bridges them in Python:

```sh
python3 examples/modbus_rtu_transport/run_pty_demo.py \
  --device build/linux-debug/examples/diag_modbus_rtu_device_cpp \
  --tester build/linux-debug/examples/diag_modbus_rtu_tester_cpp
```

CTest runs the same flow when the Modbus RTU example is enabled:

```sh
./build.py test -- \
  DIAG_BUILD_MODBUS_RTU_EXAMPLE=ON \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

Expected tester output includes:

```text
modbus_tester: opening diagnostic session on /dev/pts/... address=17
modbus_tester: identity ecosystem=7 product=42 type=3 instance=1 stage=1 component=2
modbus_tester: DTC count=2
modbus_tester: DTC 0x030101 status=0x6d severity=2 occurrences=1
modbus_tester: cleared DTC 0x030101
modbus_tester: diagnostic session complete
```

## Run With Real RS-485 Ports

Use two connected serial adapters, or run the slave on a target and the tester on
the host:

```sh
./build/linux-debug/examples/diag_modbus_rtu_device_cpp --port /dev/ttyUSB0 --address 17
./build/linux-debug/examples/diag_modbus_rtu_tester_cpp --port /dev/ttyUSB1 --address 17
```
