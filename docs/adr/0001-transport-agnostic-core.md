# ADR 0001: Keep Diagnostic Core Transport Agnostic

## Status

Accepted

## Context

The library is inspired by CAN UDS workflows, but users should be able to run it
over any transport.

## Decision

The diagnostic core will not include CAN, ISO-TP, SocketCAN, UART, or TCP
dependencies. Transport behavior is provided through `diag_transport_ops_t`.

## Consequences

- CAN support can be added as an example adapter without changing the core.
- Protocol framing remains the user's responsibility or a separate optional
  module.
- Tests can use in-memory fake transports.

