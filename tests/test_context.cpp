#include <gtest/gtest.h>

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
    struct diag_context *ctx = nullptr;
    struct diag_dtc_snapshot dtc_buffer[1];
    const struct diag_config config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 1,
        /* storage      */ {},
        /* transport    */ {},
    };

    EXPECT_EQ(diag_init(nullptr, &config, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&storage, nullptr, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&storage, &config, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextInit, AcceptsValidConfiguration)
{
    struct diag_context_storage storage = {};
    struct diag_context *ctx = nullptr;
    struct diag_dtc_snapshot dtc_buffer[2];
    const struct diag_config config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 2,
        /* storage      */ {},
        /* transport    */ {},
    };

    EXPECT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
    EXPECT_NE(ctx, nullptr);
    EXPECT_EQ(diag_deinit(ctx), DIAG_OK);
}

TEST(DiagContextInit, RejectsInvalidDtcStorage)
{
    struct diag_context_storage storage = {};
    struct diag_context *ctx = nullptr;

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
    struct diag_context *ctx = reinterpret_cast<struct diag_context *>(0xDEADBEEF);
    const struct diag_config bad_config = {
        /* dtc_buffer   */ nullptr,
        /* dtc_capacity */ 0,
        /* storage      */ {},
        /* transport    */ {},
    };

    EXPECT_EQ(diag_init(&storage, &bad_config, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(ctx, nullptr);
}

} // namespace
