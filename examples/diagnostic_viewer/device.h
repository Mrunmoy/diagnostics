#ifndef DIAGNOSTIC_VIEWER_DEVICE_H
#define DIAGNOSTIC_VIEWER_DEVICE_H

#include "example_diag_tool.h"

struct diagnostic_viewer_store
{
    uint8_t bytes[512];
    size_t  used;
};

struct diagnostic_viewer_device
{
    struct diag_context_storage    context_storage;
    struct diag_context           *ctx;
    struct diag_dtc_snapshot       dtc_records[6];
    struct diagnostic_viewer_store persistent_store;
    uint8_t                        capsule_buffer[512];
};

enum diag_result diagnostic_viewer_device_init(struct diagnostic_viewer_device *device);
enum diag_result diagnostic_viewer_device_apply_scenario(struct diagnostic_viewer_device *device,
                                                         uint32_t                         step);
enum diag_result diagnostic_viewer_device_clear(struct diagnostic_viewer_device *device,
                                                diag_dtc_id_t                    dtc_id);
enum diag_result diagnostic_viewer_device_deinit(struct diagnostic_viewer_device *device);
size_t           diagnostic_viewer_device_persisted_size(void *user);

#endif
