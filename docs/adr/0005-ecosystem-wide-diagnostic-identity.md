# ADR 0005: Support Ecosystem-Wide Diagnostic Identity

## Status

Accepted

## Context

Diagnostics may be collected from multiple devices within one product or across
multiple products in an ecosystem. A trouble code value alone is not globally
unique.

## Decision

The design will support compact identity fields that allow a diagnostic tool to
identify product, device, firmware stage, subsystem, and trouble code.

## Consequences

- Embedded records can stay compact.
- Host tools can map numeric IDs to rich descriptions.
- Device and product identity should be configured explicitly.
- The core must avoid assuming that DTC IDs are globally unique by themselves.

