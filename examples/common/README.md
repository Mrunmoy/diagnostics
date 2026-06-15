# Common Diagnostic Example Protocol

The transport examples share one diagnostic service payload so every link shows
the same device behavior:

- read compact identity.
- list registered DTCs.
- clear one DTC.
- read DTCs again and verify the clear was visible.

`example_diag_tool.c` is the C device/service side. It accepts bounded
`example_diag_frame` requests and builds bounded responses. Firmware examples can
call it directly or place a transport above it.

`example_diag_client.cpp` is the C++ PC/tester side. It owns the service workflow
and response validation. A transport example only needs to implement:

```cpp
enum diag_result exchange(const example_diag_frame& request,
                          example_diag_frame& response);
```

`exchange()` returns the transport result: timeout, framing failure, or
successful delivery. Device-level success or rejection stays in
`response.bytes[0]` as a `diag_result` and is decoded by the shared client.

That split keeps the library transport-agnostic while making each real transport
example comparable. SocketCAN, serial, TCP, and Modbus RTU should all expose the
same tester behavior and differ only in framing, timing, and link setup.

## Frame Contract

The shared service frame is intentionally small and fixed-capacity:

| Field | Meaning |
|-------|---------|
| `bytes[0]` request | service ID |
| `bytes[0]` response | `diag_result` |
| `size` | valid bytes in the frame |

The example transport boundary is one request in, one response out. If the link
cannot carry `EXAMPLE_DIAG_MAX_FRAME_SIZE` in one packet, that transport owns
segmentation and reassembly below the shared client.
