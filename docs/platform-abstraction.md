# Platform Abstraction

## Storage Interface

Storage is responsible for loading, saving, and clearing serialized diagnostic
state. The core decides what state means; the adapter decides where bytes live.

Examples:

- RAM buffer for tests
- EEPROM block on an MCU
- flash page with wear leveling
- filesystem blob on Linux
- database row in a gateway application

When state is shared between bootloader and application, the storage adapter is
also responsible for any platform-specific atomicity guarantees. For example,
an MCU flash adapter may write to an inactive page and then commit with a marker.
The diagnostic core should validate the serialized capsule before accepting it.

## Transport Interface

Transport is responsible for sending and receiving bytes. It does not interpret
diagnostic services.

Examples:

- SocketCAN adapter
- UART adapter
- TCP adapter
- in-memory loopback adapter for tests

## Future Platform Interfaces

These are intentionally deferred until tests prove they are required:

- clock/time provider
- lock/mutex provider
- allocator
- logging sink
