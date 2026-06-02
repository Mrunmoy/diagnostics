#ifndef DIAG_IDENTITY_H
#define DIAG_IDENTITY_H

#include <stdbool.h>
#include <stdint.h>

#include "diag/result.h"

#ifdef __cplusplus
extern "C"
{
#endif

// Forward compatibility marker for the compact numeric device identity layout.
#define DIAG_IDENTITY_SCHEMA_VERSION (1u)
#define DIAG_IDENTITY_ENCODED_SIZE (10u)

    struct diag_context;

    struct diag_identity
    {
        uint16_t ecosystem_id;
        uint16_t product_id;
        uint16_t device_type;
        uint8_t  device_instance;
        uint8_t  firmware_stage;
        uint8_t  firmware_component;
        uint8_t  reserved;
    };

    // clang-format off
    enum diag_result diag_identity_copy(const struct diag_identity *identity,
                                        struct diag_identity *out_identity);

    enum diag_result diag_identity_get(const struct diag_context *ctx,
                                       struct diag_identity *out_identity);
    // clang-format on

    bool diag_identity_equal(const struct diag_identity *left, const struct diag_identity *right);

#ifdef __cplusplus
}
#endif

#endif
