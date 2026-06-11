# Examples

Each example is a small reference integration for a generic embedded product
profile. Start with the closest use case, copy the feature switches, then replace
the fake storage or transport callbacks with platform code.

| Example | Use case | Main features | Persistence |
|---------|----------|---------------|-------------|
| `basic` | smallest context lifetime smoke test | core only | none |
| `sensor_node` | simple sensor device with runtime faults | DTC, identity | none |
| `io_module` | module discovered over a project transport | identity, transport | none |
| `dtc` | DTC API behavior in RAM | DTC | none |
| `lifecycle` | reset counter policy without storage | lifecycle | none |
| `identity` | compact device/product identity | identity | none |
| `adapters` | storage and transport callback wiring | storage, transport | adapter-owned |
| `process_controller` | confirmed process faults | DTC, lifecycle, storage, capsule | explicit |
| `industrial_oven` | critical thermal/reset diagnostics | DTC, lifecycle, identity, storage, capsule | explicit |
| `bootloader_app_shared` | separate bootloader and application banks | DTC, lifecycle, storage, capsule | explicit |
| `ecu_node` | complete embedded node profile | all features | explicit |

Run all examples enabled by a profile through CTest:

```sh
./build.py test -- DIAG_FEATURE_DTC=ON DIAG_FEATURE_LIFECYCLE=ON DIAG_FEATURE_IDENTITY=ON \
  DIAG_FEATURE_STORAGE=ON DIAG_FEATURE_TRANSPORT=ON DIAG_FEATURE_CAPSULE=ON
```

Inspect footprint by profile:

```sh
./build.py feature-matrix
./build.py size --dtc-capacity 8 --sections dtc,lifecycle
```
