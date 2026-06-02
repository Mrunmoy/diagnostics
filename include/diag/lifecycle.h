#ifndef DIAG_LIFECYCLE_H
#define DIAG_LIFECYCLE_H

#include <stdbool.h>
#include <stdint.h>

#include "diag/result.h"

enum diag_reset_reason
{
    DIAG_RESET_REASON_UNKNOWN = 0,
    DIAG_RESET_REASON_POWER_ON,
    DIAG_RESET_REASON_SOFTWARE,
    DIAG_RESET_REASON_WATCHDOG,
    DIAG_RESET_REASON_BROWNOUT,
    DIAG_RESET_REASON_EXTERNAL,
    DIAG_RESET_REASON_UPDATE,
    DIAG_RESET_REASON_FAULT
};

enum diag_reset_counter_policy
{
    DIAG_RESET_COUNTER_POLICY_DISABLED = 0,
    DIAG_RESET_COUNTER_POLICY_RAM_ONLY,
    DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY,
    DIAG_RESET_COUNTER_POLICY_EVERY_N,
    DIAG_RESET_COUNTER_POLICY_PLATFORM
};

enum diag_lifecycle_dirty_flag
{
    DIAG_LIFECYCLE_DIRTY_NONE = 0u,
    DIAG_LIFECYCLE_DIRTY_RESET_COUNTER = 1u
};

struct diag_lifecycle_config
{
    enum diag_reset_counter_policy reset_counter_policy;
    uint32_t reset_count_interval;
    uint32_t platform_reset_count;
};

struct diag_lifecycle_snapshot
{
    enum diag_reset_reason last_reset_reason;
    enum diag_reset_counter_policy reset_counter_policy;
    uint32_t reset_count;
    uint32_t abnormal_reset_count;
    uint32_t dirty_flags;
    bool persist_requested;
};

struct diag_context;

enum diag_result diag_lifecycle_observe_reset(struct diag_context *ctx,
                                              enum diag_reset_reason reason);

enum diag_result diag_lifecycle_get(const struct diag_context *ctx,
                                    struct diag_lifecycle_snapshot *out);

enum diag_result diag_lifecycle_clear_dirty(struct diag_context *ctx, uint32_t dirty_flags);

#endif
