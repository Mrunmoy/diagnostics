#ifndef DIAG_CONTEXT_H
#define DIAG_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/dtc.h"
#include "diag/identity.h"
#include "diag/lifecycle.h"
#include "diag/result.h"
#include "diag/storage.h"
#include "diag/transport.h"

#define DIAG_CONTEXT_STORAGE_SIZE 128u

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
    union
    {
        uint8_t bytes[DIAG_CONTEXT_STORAGE_SIZE];
        void *align_pointer;
        size_t align_size;
        uint32_t align_u32;
        // Carries the widest fundamental alignment so the storage is suitably
        // aligned for the private context regardless of its layout. A
        // compile-time alignment check in the implementation enforces this.
        long double align_max;
    } data;
};

enum diag_result diag_init(struct diag_context_storage *context_storage,
                           const struct diag_config *config, struct diag_context **out_ctx);
enum diag_result diag_deinit(struct diag_context *ctx);

enum diag_result diag_save(struct diag_context *ctx);
enum diag_result diag_load(struct diag_context *ctx);

#endif
