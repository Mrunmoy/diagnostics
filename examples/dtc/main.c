#include "diag/diag.h"

#include <stdio.h>

int main(void)
{
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    struct diag_dtc_snapshot     records[2] = {0};
    struct diag_dtc_snapshot     snapshot = {0};
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = records,
        .capacity = 2u,
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_init failed\n");
        return 1;
    }

    if (diag_dtc_attach(ctx, &dtc_config) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_attach failed\n");
        return 1;
    }

    if (diag_dtc_register_fault(ctx, 10u, 0x000100u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_register_fault failed\n");
        return 1;
    }

    if (diag_dtc_set_fault_test_failed(ctx, 10u) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_set_fault_test_failed failed\n");
        return 1;
    }

    if (diag_dtc_operation_cycle(ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_operation_cycle failed\n");
        return 1;
    }

    if (diag_dtc_get(ctx, 0x000100u, &snapshot) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_get failed\n");
        return 1;
    }

    if ((snapshot.status & DIAG_DTC_STATUS_CONFIRMED) == 0u)
    {
        fprintf(stderr, "expected DTC to be confirmed\n");
        return 1;
    }

    if (diag_deinit(ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_deinit failed\n");
        return 1;
    }

    return 0;
}
