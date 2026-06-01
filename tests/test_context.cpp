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
    diag_context_t ctx;
    diag_dtc_snapshot_t dtc_buffer[1];
    const diag_config_t config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 1,
        /* storage      */ {},
        /* transport    */ {},
    };

    EXPECT_EQ(diag_init(nullptr, &config), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&ctx, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextInit, AcceptsValidConfiguration)
{
    diag_context_t ctx;
    diag_dtc_snapshot_t dtc_buffer[2];
    const diag_config_t config = {
        /* dtc_buffer   */ dtc_buffer,
        /* dtc_capacity */ 2,
        /* storage      */ {},
        /* transport    */ {},
    };

    EXPECT_EQ(diag_init(&ctx, &config), DIAG_OK);
    EXPECT_TRUE(ctx.initialized);
    EXPECT_EQ(ctx.dtc_count, 0u);
}

} // namespace
