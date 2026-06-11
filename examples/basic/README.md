# Basic Example

Use this when you are bringing the library into a new build system and want the
smallest possible smoke test. It proves that a caller-owned
`struct diag_context_storage` can hold the opaque context and that no optional
module is required.

The example does not register DTCs, attach identity, attach storage, or use a
transport. That is intentional: it is the baseline for code-size comparisons and
for checking that the library can be linked before product diagnostics are
designed.

## Read The Code

Start in `main.c`:

- `struct diag_context_storage storage` is the memory the library uses.
- `diag_init()` creates the opaque context inside that memory.
- `diag_deinit()` releases runtime state without touching storage.

## Build And Run

```sh
./build.py build -- \
  DIAG_FEATURE_DTC=OFF \
  DIAG_FEATURE_LIFECYCLE=OFF \
  DIAG_FEATURE_IDENTITY=OFF \
  DIAG_FEATURE_STORAGE=OFF \
  DIAG_FEATURE_TRANSPORT=OFF \
  DIAG_FEATURE_CAPSULE=OFF

./build/linux-debug/examples/diag_basic_example
```

Success means the context lifetime path works with every optional feature
compiled out.
