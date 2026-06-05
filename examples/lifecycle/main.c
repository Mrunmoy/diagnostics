#include "diag/diag.h"

#include <stdio.h>

int main(void)
{
    struct diag_context_storage        storage = {0};
    struct diag_context               *ctx = NULL;
    struct diag_lifecycle_snapshot     snapshot = {0};
    const struct diag_config           config = {0};
    const struct diag_lifecycle_config lifecycle_config = {
        .reset_counter_policy = DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY,
        .reset_count_interval = 0u,
        .platform_reset_count = 0u,
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_init failed\n");
        return 1;
    }

    if (diag_lifecycle_attach(ctx, &lifecycle_config) != DIAG_OK)
    {
        fprintf(stderr, "diag_lifecycle_attach failed\n");
        return 1;
    }

    if (diag_lifecycle_observe_reset(ctx, DIAG_RESET_REASON_WATCHDOG) != DIAG_OK)
    {
        fprintf(stderr, "diag_lifecycle_observe_reset failed\n");
        return 1;
    }

    if (diag_lifecycle_get(ctx, &snapshot) != DIAG_OK)
    {
        fprintf(stderr, "diag_lifecycle_get failed\n");
        return 1;
    }

    if (!snapshot.persist_requested || snapshot.abnormal_reset_count != 1u)
    {
        fprintf(stderr, "unexpected lifecycle snapshot\n");
        return 1;
    }

    if (diag_deinit(ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_deinit failed\n");
        return 1;
    }

    return 0;
}
