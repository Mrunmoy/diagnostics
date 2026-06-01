#include "diag_context_internal.h"

enum diag_result diag_init(struct diag_context_storage *storage, const struct diag_config *config,
                           struct diag_context **out_ctx)
{
    struct diag_context *ctx;

    // Clear the out-parameter up front so a failed init never leaves the caller
    // holding a stale context pointer from a previous successful call.
    if (out_ctx != 0)
    {
        *out_ctx = 0;
    }

    if (storage == 0 || config == 0 || out_ctx == 0 || config->dtc_buffer == 0 ||
        config->dtc_capacity == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    ctx = (struct diag_context *)(void *)&storage->data.bytes[0];
    ctx->initialized = true;
    ctx->config = *config;
    ctx->dtc_count = 0;
    *out_ctx = ctx;

    return DIAG_OK;
}

enum diag_result diag_deinit(struct diag_context *ctx)
{
    if (ctx == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    ctx->initialized = false;
    ctx->dtc_count = 0;

    return DIAG_OK;
}

enum diag_result diag_save(struct diag_context *ctx)
{
    (void)ctx;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_load(struct diag_context *ctx)
{
    (void)ctx;
    return DIAG_ERROR_NOT_INITIALIZED;
}
