#include "diag/diag.h"

#include <stdio.h>

int main(void)
{
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    struct diag_dtc_snapshot     dtc_records[2] = {0};
    struct diag_dtc_snapshot     sensor_fault = {0};
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = dtc_records,
        .capacity = 2u,
    };
    const struct diag_identity identity = {
        .ecosystem_id = 1u,
        .product_id = 10u,
        .device_type = 1u,
        .device_instance = 7u,
        .firmware_stage = 1u,
        .firmware_component = 1u,
        .reserved = 0u,
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_init failed\n");
        return 1;
    }

    if (diag_identity_attach(ctx, &identity) != DIAG_OK)
    {
        fprintf(stderr, "diag_identity_attach failed\n");
        return 1;
    }

    if (diag_dtc_attach(ctx, &dtc_config) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_attach failed\n");
        return 1;
    }

    if (diag_dtc_register_fault(ctx, 1u, 0x010001u, DIAG_DTC_SEVERITY_WARNING) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_register_fault failed\n");
        return 1;
    }

    if (diag_dtc_set_fault_test_failed(ctx, 1u) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_set_fault_test_failed failed\n");
        return 1;
    }

    if (diag_dtc_get(ctx, 0x010001u, &sensor_fault) != DIAG_OK)
    {
        fprintf(stderr, "diag_dtc_get failed\n");
        return 1;
    }

    if (!sensor_fault.active)
    {
        fprintf(stderr, "expected active sensor fault\n");
        return 1;
    }

    printf("sensor_node: active runtime DTC 0x%06lx\n", (unsigned long)sensor_fault.id);

    if (diag_deinit(ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_deinit failed\n");
        return 1;
    }

    return 0;
}
