#ifndef DIAGNOSTIC_SESSION_DEVICE_H
#define DIAGNOSTIC_SESSION_DEVICE_H

#include "protocol.h"

struct example_storage
{
    uint8_t bytes[384];
    size_t  used;
};

struct diagnostic_device
{
    struct diag_context_storage context_storage;
    struct diag_context        *ctx;
    struct diag_dtc_snapshot    dtc_records[4];
    struct example_storage      persistent_store;
    uint8_t                     capsule_buffer[384];
};

#endif
