#include "diag/context.h"

diag_result_t diag_init(diag_context_t *ctx, const diag_config_t *config)
{
    if (ctx == 0 || config == 0 || config->dtc_buffer == 0 || config->dtc_capacity == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    ctx->initialized = true;
    ctx->config = *config;
    ctx->dtc_count = 0;

    return DIAG_OK;
}

diag_result_t diag_deinit(diag_context_t *ctx)
{
    if (ctx == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    ctx->initialized = false;
    ctx->dtc_count = 0;

    return DIAG_OK;
}

diag_result_t diag_save(diag_context_t *ctx)
{
    (void)ctx;
    return DIAG_ERROR_NOT_INITIALIZED;
}

diag_result_t diag_load(diag_context_t *ctx)
{
    (void)ctx;
    return DIAG_ERROR_NOT_INITIALIZED;
}
