#ifndef DIAG_DTC_H
#define DIAG_DTC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

typedef uint32_t diag_dtc_id_t;

typedef enum diag_dtc_severity
{
    DIAG_DTC_SEVERITY_INFO = 0,
    DIAG_DTC_SEVERITY_WARNING,
    DIAG_DTC_SEVERITY_ERROR,
    DIAG_DTC_SEVERITY_CRITICAL
} diag_dtc_severity_t;

typedef struct diag_dtc_snapshot
{
    diag_dtc_id_t id;
    diag_dtc_severity_t severity;
    bool active;
    uint32_t occurrence_count;
    uint32_t active_count;
    uint32_t clear_count;
} diag_dtc_snapshot_t;

struct diag_context;

diag_result_t diag_dtc_register(struct diag_context *ctx, diag_dtc_id_t id,
                                diag_dtc_severity_t severity);

diag_result_t diag_dtc_set_active(struct diag_context *ctx, diag_dtc_id_t id);
diag_result_t diag_dtc_set_inactive(struct diag_context *ctx, diag_dtc_id_t id);
diag_result_t diag_dtc_clear(struct diag_context *ctx, diag_dtc_id_t id);
diag_result_t diag_dtc_clear_all(struct diag_context *ctx);
diag_result_t diag_dtc_reset_counter(struct diag_context *ctx, diag_dtc_id_t id);

diag_result_t diag_dtc_get(const struct diag_context *ctx, diag_dtc_id_t id,
                           diag_dtc_snapshot_t *out);

diag_result_t diag_dtc_list(const struct diag_context *ctx, diag_dtc_snapshot_t *out,
                            size_t capacity, size_t *count);

#endif
