#ifndef GRAFANA_READER_DEVICE_H
#define GRAFANA_READER_DEVICE_H

#include "example_diag_tool.h"

struct grafana_reader_store
{
    uint8_t bytes[384];
    size_t  used;
};

struct grafana_reader_device
{
    struct diag_context_storage context_storage;
    struct diag_context        *ctx;
    struct diag_dtc_snapshot    dtc_records[4];
    struct grafana_reader_store persistent_store;
    uint8_t                     capsule_buffer[384];
};

enum diag_result grafana_reader_device_init(struct grafana_reader_device *device);
enum diag_result grafana_reader_device_deinit(struct grafana_reader_device *device);
size_t           grafana_reader_device_persisted_size(void *user);

#endif
