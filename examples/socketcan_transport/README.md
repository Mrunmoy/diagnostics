# SocketCAN Transport Example

This example moves the existing diagnostic request/response workflow onto Linux
SocketCAN. It uses your `Mrunmoy/socket-can` C++17 wrapper on the PC side and
keeps the diagnostics library itself as C99.

Two processes run on the same CAN interface:

- `diag_socketcan_device_cpp` simulates embedded firmware. It owns the C
  diagnostics context, DTCs, identity, and persistent capsule.
- `diag_socketcan_tester_cpp` acts as the PC diagnostic tester. It sends
  requests over CAN FD, reads responses, prints identity/DTCs, clears one DTC,
  and verifies that the clear was observable.

The tester uses `examples/common/example_diag_client.cpp` for the actual
diagnostic workflow. SocketCAN-specific code only opens the CAN interface and
exchanges one request frame for one response frame.

The demo uses one diagnostic payload per CAN FD frame:

| Direction | CAN ID | Payload |
|-----------|--------|---------|
| tester to device | `0x650` | `example_diag_frame` request bytes |
| device to tester | `0x651` | `example_diag_frame` response bytes |

CAN FD is used because DTC list responses can exceed the 8-byte classic CAN
payload. A production transport can add segmentation, counters, addressing, and
timeouts around the same diagnostic service payloads.

## Build

From the repository root:

```sh
./build.py build -- \
  DIAG_BUILD_SOCKETCAN_EXAMPLE=ON \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

This example vendors `SocketCan.hpp` from `https://github.com/Mrunmoy/socket-can`
under `third_party/socket-can/` with its MIT license so the example does not
need network access during CMake configure.

## Run With vcan0

The Docker/devcontainer image creates `vcan0` automatically when it starts. Build
inside the container with:

```sh
docker compose run --rm diagnostics-dev \
  ./build.py test --preset container-debug -- \
  DIAG_BUILD_SOCKETCAN_EXAMPLE=ON \
  DIAG_FEATURE_DTC=ON \
  DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON \
  DIAG_FEATURE_CAPSULE=ON
```

If you are running directly on a Linux host instead of Docker, create `vcan0`
once if your machine does not already have it:

```sh
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

Run the automated two-process demo:

```sh
python3 examples/socketcan_transport/run_vcan_demo.py \
  --device build/linux-debug/examples/diag_socketcan_device_cpp \
  --tester build/linux-debug/examples/diag_socketcan_tester_cpp
```

Or run the processes manually in two terminals:

```sh
./build/linux-debug/examples/diag_socketcan_device_cpp --interface vcan0
./build/linux-debug/examples/diag_socketcan_tester_cpp --interface vcan0
```

Expected tester output includes:

```text
socketcan_tester: opening diagnostic session on vcan0
socketcan_tester: identity ecosystem=7 product=42 type=3 instance=1 stage=1 component=2
socketcan_tester: DTC count=2
socketcan_tester: DTC 0x030101 status=0x6d severity=2 occurrences=1
socketcan_tester: cleared DTC 0x030101
socketcan_tester: diagnostic session complete
```
