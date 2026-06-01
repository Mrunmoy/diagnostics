# Ecosystem Identity

## Problem

One product may contain multiple devices that cooperate:

- main controller
- motor controller
- battery module
- display
- sensor node
- gateway
- bootloader and application images on each device

Each device may report trouble codes. A diagnostic tool must identify where a
code came from, not only what numeric code was reported.

## Design Direction

Diagnostics should be identified by a tuple:

```text
ecosystem id
product id
device type
device instance
firmware stage
subsystem
trouble code
```

The core can keep the first implementation small, but the data model should not
prevent this richer identity from being layered on top.

## Local Code vs Global Code

A local code is meaningful inside one firmware image:

```text
subsystem = storage
code      = write_failed
```

A global diagnostic identity is meaningful across a fleet or ecosystem:

```text
product       = inverter_v2
device_type   = power_board
device_index  = 2
stage         = application
subsystem     = storage
code          = write_failed
```

The library should allow local embedded firmware to stay efficient while giving
external tools enough metadata to build a globally unique identity.

## Suggested Identity Fields

The first design should reserve space for:

- `vendor_id`
- `product_id`
- `device_type`
- `device_instance`
- `firmware_stage`
- `firmware_component`
- `firmware_version`

These fields should be configured by the application, not discovered by the
core.

## Efficiency Rules

- Do not store repeated long strings in every DTC record.
- Prefer compact numeric identity fields in embedded memory.
- Let host tools map numeric IDs to human-readable descriptions.
- Keep description tables optional and outside the core.
- Allow the same diagnostic database to describe many products.

## Tooling Implication

The embedded library should report compact facts. A richer host-side diagnostic
database can translate those facts into names, documentation, service actions,
and product-specific troubleshooting flows.

