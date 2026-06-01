# TDD Strategy

## Workflow

1. Write a focused failing test for one behavior.
2. Implement the smallest production code needed to pass.
3. Refactor while tests remain green.
4. Add edge-case tests before expanding the API.

## Test Layers

- Unit tests: individual API behavior with fake storage and transport.
- Contract tests: verify adapter behavior against the platform interface.
- Integration tests: verify protocol adapters against the diagnostic core.

## First Test Backlog

- context initialization rejects null arguments.
- context initialization stores config and adapter pointers.
- registering a DTC makes it retrievable.
- registering duplicate DTC returns a deterministic error.
- setting active increments occurrence count.
- setting inactive preserves historical counters.
- clearing one DTC resets status according to policy.
- clearing all DTCs affects every registered code.
- save calls the configured storage adapter.
- load validates serialized data before mutating live state.
- bootloader and application banks do not overwrite each other's records.
- loading an older diagnostic capsule preserves compatible records.
- loading a capsule with an invalid integrity check rejects the data.
- initializing with zero capacity is rejected.
- registering more than configured capacity returns `DIAG_ERROR_CAPACITY`.
- core tests prove no API requires dynamic allocation.
- ecosystem identity fields are preserved through save/load.
