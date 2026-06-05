#include <gtest/gtest.h>

extern "C"
{
#include "diag/diag.h"
}

namespace
{

static_assert(sizeof(struct diag_identity) == DIAG_IDENTITY_ENCODED_SIZE,
              "diag_identity must remain compact for capsule metadata");

TEST(DiagIdentity, CopiesFixedWidthNumericFields)
{
    const struct diag_identity identity = {
        /* ecosystem_id       */ 0x1001u,
        /* product_id         */ 0x2002u,
        /* device_type        */ 0x3003u,
        /* device_instance    */ 0x04u,
        /* firmware_stage     */ 0x05u,
        /* firmware_component */ 0x06u,
        /* reserved           */ 0u,
    };
    struct diag_identity copied = {};

    EXPECT_EQ(diag_identity_copy(&identity, &copied), DIAG_OK);
    EXPECT_EQ(copied.ecosystem_id, 0x1001u);
    EXPECT_EQ(copied.product_id, 0x2002u);
    EXPECT_EQ(copied.device_type, 0x3003u);
    EXPECT_EQ(copied.device_instance, 0x04u);
    EXPECT_EQ(copied.firmware_stage, 0x05u);
    EXPECT_EQ(copied.firmware_component, 0x06u);
    EXPECT_EQ(copied.reserved, 0u);
}

TEST(DiagIdentity, RejectsNullCopyArguments)
{
    const struct diag_identity identity = {};
    struct diag_identity       copied = {};

    EXPECT_EQ(diag_identity_copy(nullptr, &copied), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_identity_copy(&identity, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagIdentity, GetsIdentityConfiguredOnContext)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    const struct diag_config    config = {};
    const struct diag_identity  configured_identity = {
         /* ecosystem_id       */ 0x0001u,
        /* product_id         */ 0x0002u,
        /* device_type        */ 0x0003u,
        /* device_instance    */ 0x04u,
        /* firmware_stage     */ 0x05u,
        /* firmware_component */ 0x06u,
        /* reserved           */ 0u,
    };
    struct diag_identity identity = {};

    ASSERT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
    ASSERT_EQ(diag_identity_attach(ctx, &configured_identity), DIAG_OK);
    EXPECT_EQ(diag_identity_get(ctx, &identity), DIAG_OK);
    EXPECT_EQ(identity.ecosystem_id, 0x0001u);
    EXPECT_EQ(identity.product_id, 0x0002u);
    EXPECT_EQ(identity.device_type, 0x0003u);
    EXPECT_EQ(identity.device_instance, 0x04u);
    EXPECT_EQ(identity.firmware_stage, 0x05u);
    EXPECT_EQ(identity.firmware_component, 0x06u);
}

TEST(DiagIdentity, RejectsInvalidGetArguments)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    struct diag_identity        identity = {};

    EXPECT_EQ(diag_identity_get(nullptr, &identity), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_identity_get(ctx, nullptr), DIAG_ERROR_INVALID_ARGUMENT);

    const struct diag_config config = {};

    ASSERT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
    EXPECT_EQ(diag_deinit(ctx), DIAG_OK);
    EXPECT_EQ(diag_identity_get(ctx, &identity), DIAG_ERROR_NOT_INITIALIZED);
}

TEST(DiagIdentity, RejectsAttachAndGetBeforeValidContext)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    const struct diag_config    config = {};
    const struct diag_identity  identity = {};
    struct diag_identity        out = {};

    ASSERT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);

    EXPECT_EQ(diag_identity_attach(nullptr, &identity), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_identity_attach(ctx, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_identity_get(ctx, &out), DIAG_ERROR_NOT_INITIALIZED);

    ASSERT_EQ(diag_deinit(ctx), DIAG_OK);
    EXPECT_EQ(diag_identity_attach(ctx, &identity), DIAG_ERROR_NOT_INITIALIZED);
}

TEST(DiagIdentity, ComparesAllFields)
{
    const struct diag_identity left = {
        /* ecosystem_id       */ 1u,
        /* product_id         */ 2u,
        /* device_type        */ 3u,
        /* device_instance    */ 4u,
        /* firmware_stage     */ 5u,
        /* firmware_component */ 6u,
        /* reserved           */ 0u,
    };
    struct diag_identity right = left;

    EXPECT_TRUE(diag_identity_equal(&left, &right));
    EXPECT_FALSE(diag_identity_equal(nullptr, &right));
    EXPECT_FALSE(diag_identity_equal(&left, nullptr));

    right.firmware_component = 7u;
    EXPECT_FALSE(diag_identity_equal(&left, &right));
}

} // namespace
