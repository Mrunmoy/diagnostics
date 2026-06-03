#include <gtest/gtest.h>

#include <cstdint>

// The library under test is C99. Pull its public headers in with C linkage so
// the C++ test translation unit links against the unmangled symbols.
extern "C"
{
#include "diag/diag.h"
}

namespace
{

TEST(DiagContextInit, RejectsNullArguments)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    struct diag_dtc_snapshot    dtc_buffer[1];
    // clang-format off
    const struct diag_config    config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 1,
        /* storage      */ {},
        /* transport    */ {},
    };
    // clang-format on

    EXPECT_EQ(diag_init(nullptr, &config, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&storage, nullptr, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&storage, &config, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextInit, AcceptsValidConfiguration)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    struct diag_dtc_snapshot    dtc_buffer[2];
    // clang-format off
    const struct diag_config    config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 2,
        /* storage      */ {},
        /* transport    */ {},
    };
    // clang-format on

    EXPECT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
    EXPECT_NE(ctx, nullptr);
    EXPECT_EQ(diag_deinit(ctx), DIAG_OK);
}

TEST(DiagContextLifetime, DeinitRejectsNullContext)
{
    EXPECT_EQ(diag_deinit(nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextLifetime, DeinitInvalidatesContextForApiUse)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    struct diag_dtc_snapshot    dtc_buffer[1];
    // clang-format off
    const struct diag_config    config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 1,
        /* storage      */ {},
        /* transport    */ {},
    };
    // clang-format on
    struct diag_identity identity = {};

    ASSERT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
    ASSERT_EQ(diag_deinit(ctx), DIAG_OK);

    EXPECT_EQ(diag_identity_get(ctx, &identity), DIAG_ERROR_NOT_INITIALIZED);
}

TEST(DiagContextStorage, IsAlignedForOpaqueContext)
{
    struct diag_context_storage storage = {};

    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&storage.bytes[0]) % DIAG_CONTEXT_STORAGE_ALIGN, 0u);
    EXPECT_EQ(sizeof(storage), static_cast<std::size_t>(DIAG_CONTEXT_STORAGE_SIZE));
    EXPECT_EQ(sizeof(storage.bytes), static_cast<std::size_t>(DIAG_CONTEXT_STORAGE_SIZE));
}

TEST(DiagContextInit, RejectsMisalignedStorageWithoutDereferencingIt)
{
    // clang-format off
    alignas(DIAG_CONTEXT_STORAGE_ALIGN)
        uint8_t raw[sizeof(struct diag_context_storage) + DIAG_CONTEXT_STORAGE_ALIGN] = {};
    // clang-format on
    struct diag_context     *ctx = reinterpret_cast<struct diag_context *>(0xDEADBEEF);
    struct diag_dtc_snapshot dtc_buffer[1];
    const struct diag_config config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 1,
        /* storage      */ {},
        /* transport    */ {},
    };

    auto *misaligned_storage = reinterpret_cast<struct diag_context_storage *>(&raw[1]);

    ASSERT_NE(reinterpret_cast<std::uintptr_t>(misaligned_storage) % DIAG_CONTEXT_STORAGE_ALIGN,
              0u);
    EXPECT_EQ(diag_init(misaligned_storage, &config, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(ctx, nullptr);
}

TEST(DiagContextInit, RejectsInvalidDtcStorage)
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;

    const struct diag_config missing_buffer = {
        /* dtc_buffer   */ nullptr,
        /* dtc_capacity */ 1,
        /* storage      */ {},
        /* transport    */ {},
    };

    struct diag_dtc_snapshot dtc_buffer[1];
    const struct diag_config missing_capacity = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 0,
        /* storage      */ {},
        /* transport    */ {},
    };

    EXPECT_EQ(diag_init(&storage, &missing_buffer, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&storage, &missing_capacity, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextInit, ClearsOutContextOnFailure)
{
    // A failed init must leave the out-parameter in a known state so a stale
    // pointer from a previous successful init cannot be reused by accident.
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = reinterpret_cast<struct diag_context *>(0xDEADBEEF);
    // clang-format off
    const struct diag_config    bad_config = {
        /* dtc_buffer   */ nullptr,
        /* dtc_capacity */ 0,
        /* storage      */ {},
        /* transport    */ {},
    };
    // clang-format on

    EXPECT_EQ(diag_init(&storage, &bad_config, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(ctx, nullptr);
}

} // namespace
