#include "diag/dtc.h"

enum diag_result diag_dtc_register(struct diag_context *ctx, diag_dtc_id_t id,
                                   enum diag_dtc_severity severity)
{
    (void)ctx;
    (void)id;
    (void)severity;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_set_active(struct diag_context *ctx, diag_dtc_id_t id)
{
    (void)ctx;
    (void)id;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_set_inactive(struct diag_context *ctx, diag_dtc_id_t id)
{
    (void)ctx;
    (void)id;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_clear(struct diag_context *ctx, diag_dtc_id_t id)
{
    (void)ctx;
    (void)id;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_clear_all(struct diag_context *ctx)
{
    (void)ctx;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_reset_counter(struct diag_context *ctx, diag_dtc_id_t id)
{
    (void)ctx;
    (void)id;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_get(const struct diag_context *ctx, diag_dtc_id_t id,
                              struct diag_dtc_snapshot *out)
{
    (void)ctx;
    (void)id;
    (void)out;
    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_dtc_list(const struct diag_context *ctx, struct diag_dtc_snapshot *out,
                               size_t capacity, size_t *count)
{
    (void)ctx;
    (void)out;
    (void)capacity;
    (void)count;
    return DIAG_ERROR_NOT_INITIALIZED;
}
