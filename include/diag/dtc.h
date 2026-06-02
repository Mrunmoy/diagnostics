#ifndef DIAG_DTC_H
#define DIAG_DTC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

typedef uint32_t diag_dtc_id_t;

// Forward compatibility marker for host/catalog interpretation of DTC IDs.
#define DIAG_DTC_NAMESPACE_VERSION (1u)

enum diag_dtc_severity
{
    DIAG_DTC_SEVERITY_INFO = 0,
    DIAG_DTC_SEVERITY_WARNING,
    DIAG_DTC_SEVERITY_ERROR,
    DIAG_DTC_SEVERITY_CRITICAL
};

struct diag_dtc_snapshot
{
    diag_dtc_id_t id;
    enum diag_dtc_severity severity;
    bool active;
    uint32_t occurrence_count;
    uint32_t active_count;
    uint32_t clear_count;
};

struct diag_context;

enum diag_result diag_dtc_register(struct diag_context *ctx, diag_dtc_id_t id,
                                   enum diag_dtc_severity severity);

enum diag_result diag_dtc_set_active(struct diag_context *ctx, diag_dtc_id_t id);
enum diag_result diag_dtc_set_inactive(struct diag_context *ctx, diag_dtc_id_t id);
enum diag_result diag_dtc_clear(struct diag_context *ctx, diag_dtc_id_t id);
enum diag_result diag_dtc_clear_all(struct diag_context *ctx);
enum diag_result diag_dtc_reset_counter(struct diag_context *ctx, diag_dtc_id_t id);

enum diag_result diag_dtc_get(const struct diag_context *ctx, diag_dtc_id_t id,
                              struct diag_dtc_snapshot *out);

enum diag_result diag_dtc_list(const struct diag_context *ctx, struct diag_dtc_snapshot *out,
                               size_t capacity, size_t *count);

#endif
