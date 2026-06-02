#ifndef DIAG_CONTEXT_H
#define DIAG_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/compiler.h"
#include "diag/dtc.h"
#include "diag/identity.h"
#include "diag/lifecycle.h"
#include "diag/result.h"
#include "diag/storage.h"
#include "diag/transport.h"

#define DIAG_CONTEXT_STORAGE_SIZE 128u
#define DIAG_CONTEXT_STORAGE_ALIGN 8u

struct diag_config
{
    struct diag_dtc_snapshot *dtc_buffer;
    size_t dtc_capacity;
    struct diag_storage storage;
    struct diag_transport transport;
    struct diag_identity identity;
    struct diag_lifecycle_config lifecycle;
};

struct diag_context;

struct diag_context_storage
{
    DIAG_ALIGNAS_PREFIX(DIAG_CONTEXT_STORAGE_ALIGN)
    uint8_t bytes[DIAG_CONTEXT_STORAGE_SIZE] DIAG_ALIGNAS_SUFFIX(DIAG_CONTEXT_STORAGE_ALIGN);
};

enum diag_result diag_init(struct diag_context_storage *context_storage,
                           const struct diag_config *config, struct diag_context **out_ctx);
enum diag_result diag_deinit(struct diag_context *ctx);

enum diag_result diag_save(struct diag_context *ctx);
enum diag_result diag_load(struct diag_context *ctx);

#endif
