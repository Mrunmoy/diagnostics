#include "diag_context_internal.h"

#include <stdint.h>

static int diag_context_storage_is_aligned(const struct diag_context_storage *storage)
{
    return ((uintptr_t)(const void *)storage % DIAG_CONTEXT_STORAGE_ALIGN) == 0u;
}

enum diag_result diag_init(struct diag_context_storage *context_storage,
                           const struct diag_config *config, struct diag_context **out_ctx)
{
    struct diag_context *ctx;

    // Clear the out-parameter up front so a failed init never leaves the caller
    // holding a stale context pointer from a previous successful call.
    if (out_ctx != NULL)
    {
        *out_ctx = NULL;
    }

    if (context_storage == NULL || config == NULL || out_ctx == NULL ||
        config->dtc_buffer == NULL || config->dtc_capacity == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (!diag_context_storage_is_aligned(context_storage))
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    ctx = (struct diag_context *)(void *)context_storage;
    ctx->initialized = true;
    ctx->config = *config;
    ctx->dtc_count = 0u;
    ctx->last_reset_reason = DIAG_RESET_REASON_UNKNOWN;
    if (config->lifecycle.reset_counter_policy == DIAG_RESET_COUNTER_POLICY_PLATFORM)
    {
        ctx->reset_count = config->lifecycle.platform_reset_count;
    }
    else
    {
        ctx->reset_count = 0u;
    }
    ctx->abnormal_reset_count = 0u;
    ctx->lifecycle_dirty_flags = DIAG_LIFECYCLE_DIRTY_NONE;
    *out_ctx = ctx;

    return DIAG_OK;
}

enum diag_result diag_deinit(struct diag_context *ctx)
{
    if (ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    ctx->initialized = false;
    ctx->dtc_count = 0u;
    ctx->last_reset_reason = DIAG_RESET_REASON_UNKNOWN;
    ctx->reset_count = 0u;
    ctx->abnormal_reset_count = 0u;
    ctx->lifecycle_dirty_flags = DIAG_LIFECYCLE_DIRTY_NONE;

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
