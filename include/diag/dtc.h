#ifndef DIAG_DTC_H
#define DIAG_DTC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

typedef uint32_t diag_dtc_id_t;
typedef uint32_t diag_local_fault_id_t;

// Forward compatibility marker for host/catalog interpretation of DTC IDs.
#define DIAG_DTC_NAMESPACE_VERSION (1u)

// Operation-cycle lifecycle defaults, used when the matching config field is 0.
// A DTC confirms after this many consecutive failed operation cycles, and ages
// out (heals) after this many consecutive clean operation cycles while confirmed.
#define DIAG_DTC_DEFAULT_CONFIRMATION_THRESHOLD (1u)
#define DIAG_DTC_DEFAULT_AGING_THRESHOLD (40u)

// Operation-cycle lifecycle tuning. A zero field selects the matching default
// above, so a zero-initialized config keeps standard behavior.
struct diag_dtc_config
{
    uint8_t  confirmation_threshold;
    uint16_t aging_threshold;
};

enum diag_dtc_status_bit
{
    DIAG_DTC_STATUS_TEST_FAILED = 1u << 0u,
    DIAG_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE = 1u << 1u,
    DIAG_DTC_STATUS_PENDING = 1u << 2u,
    DIAG_DTC_STATUS_CONFIRMED = 1u << 3u,
    DIAG_DTC_STATUS_TEST_NOT_COMPLETED_SINCE_CLEAR = 1u << 4u,
    DIAG_DTC_STATUS_TEST_FAILED_SINCE_CLEAR = 1u << 5u,
    DIAG_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE = 1u << 6u,
    DIAG_DTC_STATUS_WARNING_INDICATOR_REQUESTED = 1u << 7u
};

enum diag_dtc_severity
{
    DIAG_DTC_SEVERITY_INFO = 0,
    DIAG_DTC_SEVERITY_WARNING,
    DIAG_DTC_SEVERITY_ERROR,
    DIAG_DTC_SEVERITY_CRITICAL
};

struct diag_dtc_snapshot
{
    diag_dtc_id_t          id;
    diag_local_fault_id_t  local_fault_id;
    enum diag_dtc_severity severity;
    bool                   active;
    bool                   failed_this_cycle;
    uint8_t                status;
    uint8_t                failed_cycle_count;
    uint16_t               aging_counter;
    uint32_t               occurrence_count;
    uint32_t               active_count;
    uint32_t               clear_count;
};

struct diag_context;

enum diag_result diag_dtc_register(struct diag_context *ctx, diag_dtc_id_t id,
                                   enum diag_dtc_severity severity);

// clang-format off
enum diag_result diag_dtc_register_fault(struct diag_context *ctx,
                                         diag_local_fault_id_t local_fault_id,
                                         diag_dtc_id_t id,
                                         enum diag_dtc_severity severity);
// clang-format on

enum diag_result diag_dtc_set_active(struct diag_context *ctx, diag_dtc_id_t id);
enum diag_result diag_dtc_set_inactive(struct diag_context *ctx, diag_dtc_id_t id);
// clang-format off
enum diag_result diag_dtc_set_fault_test_failed(struct diag_context *ctx,
                                                diag_local_fault_id_t local_fault_id);
enum diag_result diag_dtc_set_fault_test_passed(struct diag_context *ctx,
                                                diag_local_fault_id_t local_fault_id);
// clang-format on
enum diag_result diag_dtc_clear(struct diag_context *ctx, diag_dtc_id_t id);
enum diag_result diag_dtc_clear_all(struct diag_context *ctx);
enum diag_result diag_dtc_reset_counter(struct diag_context *ctx, diag_dtc_id_t id);

// Advance every registered DTC by one completed operation cycle. This drives the
// `test_failed -> pending -> confirmed -> aged` lifecycle:
//   - a DTC that failed during the cycle counts toward confirmation and, once the
//     confirmation threshold is reached, becomes CONFIRMED;
//   - a clean cycle clears PENDING; a confirmed DTC that stays clean for the aging
//     threshold ages out (heals) back to a cleared state;
//   - the per-cycle "failed this operation cycle" latch resets and
//     "test not completed this operation cycle" is set for the new cycle.
enum diag_result diag_dtc_operation_cycle(struct diag_context *ctx);

enum diag_result diag_dtc_get(const struct diag_context *ctx, diag_dtc_id_t id,
                              struct diag_dtc_snapshot *out);
// clang-format off
enum diag_result diag_dtc_get_by_fault(const struct diag_context *ctx,
                                       diag_local_fault_id_t local_fault_id,
                                       struct diag_dtc_snapshot *out);
// clang-format on
enum diag_result diag_dtc_get_status(const struct diag_context *ctx, diag_dtc_id_t id,
                                     uint8_t *out_status);

enum diag_result diag_dtc_list(const struct diag_context *ctx, struct diag_dtc_snapshot *out,
                               size_t capacity, size_t *count);

#endif
