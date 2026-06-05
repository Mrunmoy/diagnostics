#include "diag/diag.h"

#include <stdio.h>

int main(void)
{
    struct diag_context_storage storage = {0};
    struct diag_context        *ctx = NULL;
    struct diag_identity        out = {0};
    const struct diag_config    config = {0};
    const struct diag_identity  identity = {
         .ecosystem_id = 1u,
         .product_id = 2u,
         .device_type = 3u,
         .device_instance = 4u,
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

    if (diag_identity_get(ctx, &out) != DIAG_OK)
    {
        fprintf(stderr, "diag_identity_get failed\n");
        return 1;
    }

    if (!diag_identity_equal(&identity, &out))
    {
        fprintf(stderr, "identity readback mismatch\n");
        return 1;
    }

    if (diag_deinit(ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_deinit failed\n");
        return 1;
    }

    return 0;
}
