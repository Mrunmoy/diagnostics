#include "diag/identity.h"

#include "diag_context_internal.h"

// clang-format off
enum diag_result diag_identity_copy(const struct diag_identity *identity,
                                    struct diag_identity *out_identity)
// clang-format on
{
    if (identity == 0 || out_identity == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    *out_identity = *identity;

    return DIAG_OK;
}

// clang-format off
enum diag_result diag_identity_get(const struct diag_context *ctx,
                                   struct diag_identity *out_identity)
// clang-format on
{
    if (ctx == 0 || out_identity == 0)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (!ctx->initialized)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return diag_identity_copy(&ctx->config.identity, out_identity);
}

bool diag_identity_equal(const struct diag_identity *left, const struct diag_identity *right)
{
    if (left == 0 || right == 0)
    {
        return false;
    }

    return left->ecosystem_id == right->ecosystem_id && left->product_id == right->product_id &&
           left->device_type == right->device_type &&
           left->device_instance == right->device_instance &&
           left->firmware_stage == right->firmware_stage &&
           left->firmware_component == right->firmware_component &&
           left->reserved == right->reserved;
}
