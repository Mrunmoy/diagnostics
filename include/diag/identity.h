/// @file
/// Compact numeric device identity for diagnostics records.

#ifndef DIAG_IDENTITY_H
#define DIAG_IDENTITY_H

#include <stdbool.h>
#include <stdint.h>

#include "diag/result.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// Schema version for the compact numeric identity layout.
#define DIAG_IDENTITY_SCHEMA_VERSION (1u)

/// Encoded identity size in bytes when serialized by a host or future capsule layer.
#define DIAG_IDENTITY_ENCODED_SIZE (10u)

    /// Opaque diagnostics context initialized with `diag_init()`.
    struct diag_context;

    /// Compact product and firmware identity.
    ///
    /// Fields are numeric by design. Host tooling owns strings, catalogs, and
    /// rich product meaning derived from these identifiers.
    struct diag_identity
    {
        /// Vendor, ecosystem, or fleet namespace.
        uint16_t ecosystem_id;
        /// Product identifier within the ecosystem.
        uint16_t product_id;
        /// Device type or hardware class.
        uint16_t device_type;
        /// Instance index when multiple devices of the same type exist.
        uint8_t device_instance;
        /// Firmware stage such as bootloader, application, or updater.
        uint8_t firmware_stage;
        /// Firmware component identifier within the stage.
        uint8_t firmware_component;
        /// Reserved byte; initialize to zero for forward compatibility.
        uint8_t reserved;
    };

    // clang-format off
    /// Copy an identity value after validating pointers.
    enum diag_result diag_identity_copy(const struct diag_identity *identity,
                                        struct diag_identity *out_identity);

    /// Copy the identity configured for an initialized diagnostics context.
    enum diag_result diag_identity_get(const struct diag_context *ctx,
                                       struct diag_identity *out_identity);
    // clang-format on

    /// Compare two identity values for exact numeric equality.
    bool diag_identity_equal(const struct diag_identity *left, const struct diag_identity *right);

#ifdef __cplusplus
}
#endif

#endif
