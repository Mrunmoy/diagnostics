#include "diag/diag.h"

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
        return 1;
    }

    if (diag_identity_attach(ctx, &identity) != DIAG_OK)
    {
        return 1;
    }

    if (diag_identity_get(ctx, &out) != DIAG_OK)
    {
        return 1;
    }

    if (!diag_identity_equal(&identity, &out))
    {
        return 1;
    }

    return diag_deinit(ctx) == DIAG_OK ? 0 : 1;
}
