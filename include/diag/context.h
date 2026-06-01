#ifndef DIAG_CONTEXT_H
#define DIAG_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>

#include "diag/dtc.h"
#include "diag/result.h"
#include "diag/storage.h"
#include "diag/transport.h"

typedef struct diag_config
{
    diag_dtc_snapshot_t *dtc_buffer;
    size_t dtc_capacity;
    diag_storage_t storage;
    diag_transport_t transport;
} diag_config_t;

typedef struct diag_context
{
    bool initialized;
    diag_config_t config;
    size_t dtc_count;
} diag_context_t;

diag_result_t diag_init(diag_context_t *ctx, const diag_config_t *config);
diag_result_t diag_deinit(diag_context_t *ctx);

diag_result_t diag_save(diag_context_t *ctx);
diag_result_t diag_load(diag_context_t *ctx);

#endif
