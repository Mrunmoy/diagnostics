#ifndef DIAGNOSTIC_SESSION_DEVICE_H
#define DIAGNOSTIC_SESSION_DEVICE_H

#include "example_diag_tool.h"

struct session_storage
{
    uint8_t bytes[384];
    size_t  used;
};

struct diagnostic_device
{
    struct diag_context_storage context_storage;
    struct diag_context        *ctx;
    struct diag_dtc_snapshot    dtc_records[4];
    struct session_storage      persistent_store;
    uint8_t                     capsule_buffer[384];
};

enum diag_result diagnostic_device_init(struct diagnostic_device *device);
enum diag_result diagnostic_device_deinit(struct diagnostic_device *device);
size_t           diagnostic_device_persisted_size(void *user);

#endif
