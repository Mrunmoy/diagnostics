#include <stdbool.h>
#include <stdint.h>

#include "diag_context_internal.h"

static enum diag_result diag_dtc_validate_context(const struct diag_context *ctx)
{
    if (ctx == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (!ctx->initialized)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return DIAG_OK;
}

static enum diag_result diag_dtc_find_index(const struct diag_context *ctx, diag_dtc_id_t id,
                                            size_t *out_index)
{
    size_t i;

    for (i = 0u; i < ctx->dtc_count; i++)
    {
        if (ctx->config.dtc_buffer[i].id == id)
        {
            *out_index = i;
            return DIAG_OK;
        }
    }

    return DIAG_ERROR_NOT_FOUND;
}

static uint32_t diag_dtc_increment_saturating_u32(uint32_t value)
{
    if (value == UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return value + 1u;
}

enum diag_result diag_dtc_register(struct diag_context *ctx, diag_dtc_id_t id,
                                   enum diag_dtc_severity severity)
{
    struct diag_dtc_snapshot *record;
    enum diag_result result;
    size_t ignored_index;

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (diag_dtc_find_index(ctx, id, &ignored_index) == DIAG_OK)
    {
        return DIAG_ERROR_ALREADY_EXISTS;
    }

    if (ctx->dtc_count >= ctx->config.dtc_capacity)
    {
        return DIAG_ERROR_CAPACITY;
    }

    record = &ctx->config.dtc_buffer[ctx->dtc_count];
    record->id = id;
    record->severity = severity;
    record->active = false;
    record->occurrence_count = 0u;
    record->active_count = 0u;
    record->clear_count = 0u;
    ctx->dtc_count++;

    return DIAG_OK;
}

enum diag_result diag_dtc_set_active(struct diag_context *ctx, diag_dtc_id_t id)
{
    struct diag_dtc_snapshot *record;
    enum diag_result result;
    size_t index;

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = diag_dtc_find_index(ctx, id, &index);
    if (result != DIAG_OK)
    {
        return result;
    }

    record = &ctx->config.dtc_buffer[index];
    if (!record->active)
    {
        record->active = true;
        record->occurrence_count = diag_dtc_increment_saturating_u32(record->occurrence_count);
        record->active_count = diag_dtc_increment_saturating_u32(record->active_count);
    }

    return DIAG_OK;
}

enum diag_result diag_dtc_set_inactive(struct diag_context *ctx, diag_dtc_id_t id)
{
    enum diag_result result;
    size_t index;

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = diag_dtc_find_index(ctx, id, &index);
    if (result != DIAG_OK)
    {
        return result;
    }

    ctx->config.dtc_buffer[index].active = false;

    return DIAG_OK;
}

enum diag_result diag_dtc_clear(struct diag_context *ctx, diag_dtc_id_t id)
{
    struct diag_dtc_snapshot *record;
    enum diag_result result;
    size_t index;

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = diag_dtc_find_index(ctx, id, &index);
    if (result != DIAG_OK)
    {
        return result;
    }

    record = &ctx->config.dtc_buffer[index];
    if (record->active)
    {
        record->active = false;
        record->clear_count = diag_dtc_increment_saturating_u32(record->clear_count);
    }

    return DIAG_OK;
}

enum diag_result diag_dtc_clear_all(struct diag_context *ctx)
{
    enum diag_result result;
    size_t i;

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    for (i = 0u; i < ctx->dtc_count; i++)
    {
        if (ctx->config.dtc_buffer[i].active)
        {
            ctx->config.dtc_buffer[i].active = false;
            ctx->config.dtc_buffer[i].clear_count =
                diag_dtc_increment_saturating_u32(ctx->config.dtc_buffer[i].clear_count);
        }
    }

    return DIAG_OK;
}

enum diag_result diag_dtc_reset_counter(struct diag_context *ctx, diag_dtc_id_t id)
{
    struct diag_dtc_snapshot *record;
    enum diag_result result;
    size_t index;

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = diag_dtc_find_index(ctx, id, &index);
    if (result != DIAG_OK)
    {
        return result;
    }

    record = &ctx->config.dtc_buffer[index];
    record->occurrence_count = 0u;
    record->active_count = 0u;
    record->clear_count = 0u;

    return DIAG_OK;
}

enum diag_result diag_dtc_get(const struct diag_context *ctx, diag_dtc_id_t id,
                              struct diag_dtc_snapshot *out)
{
    enum diag_result result;
    size_t index;

    if (out == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = diag_dtc_find_index(ctx, id, &index);
    if (result != DIAG_OK)
    {
        return result;
    }

    *out = ctx->config.dtc_buffer[index];
    return DIAG_OK;
}

enum diag_result diag_dtc_list(const struct diag_context *ctx, struct diag_dtc_snapshot *out,
                               size_t capacity, size_t *count)
{
    enum diag_result result;
    size_t i;

    if (out == 0 || count == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    result = diag_dtc_validate_context(ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    *count = ctx->dtc_count;
    if (capacity < ctx->dtc_count)
    {
        return DIAG_ERROR_CAPACITY;
    }

    for (i = 0u; i < ctx->dtc_count; i++)
    {
        out[i] = ctx->config.dtc_buffer[i];
    }

    return DIAG_OK;
}
