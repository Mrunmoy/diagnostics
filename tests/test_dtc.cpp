#include <gtest/gtest.h>

#include <array>

extern "C"
{
#include "diag/diag.h"
}

namespace
{

struct DtcFixture : public testing::Test
{
    struct diag_context_storage storage = {};
    std::array<struct diag_dtc_snapshot, 3> dtc_buffer = {};
    struct diag_context *ctx = nullptr;

    void SetUp() override
    {
        const struct diag_config config = {
            /* dtc_buffer   */ dtc_buffer.data(),
            /* dtc_capacity */ dtc_buffer.size(),
            /* storage      */ {},
            /* transport    */ {},
        };

        ASSERT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
        ASSERT_NE(ctx, nullptr);
    }
};

TEST(DiagDtc, RejectsInvalidArguments)
{
    struct diag_dtc_snapshot snapshot = {};
    size_t count = 0;

    EXPECT_EQ(diag_dtc_register(nullptr, 1, DIAG_DTC_SEVERITY_ERROR), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_set_active(nullptr, 1), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_set_inactive(nullptr, 1), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_clear(nullptr, 1), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_clear_all(nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_get(nullptr, 1, &snapshot), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_list(nullptr, &snapshot, 1, &count), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST_F(DtcFixture, RegistersAndGetsDtc)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 0x1234u, DIAG_DTC_SEVERITY_CRITICAL), DIAG_OK);

    EXPECT_EQ(diag_dtc_get(ctx, 0x1234u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.id, 0x1234u);
    EXPECT_EQ(snapshot.severity, DIAG_DTC_SEVERITY_CRITICAL);
    EXPECT_FALSE(snapshot.active);
    EXPECT_EQ(snapshot.occurrence_count, 0u);
    EXPECT_EQ(snapshot.active_count, 0u);
    EXPECT_EQ(snapshot.clear_count, 0u);
}

TEST_F(DtcFixture, RejectsNullOutputArguments)
{
    struct diag_dtc_snapshot snapshot = {};
    size_t count = 0;

    EXPECT_EQ(diag_dtc_get(ctx, 1u, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_list(ctx, nullptr, 1, &count), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_list(ctx, &snapshot, 1, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST_F(DtcFixture, RejectsDuplicateAndCapacityOverflow)
{
    EXPECT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_INFO), DIAG_OK);
    EXPECT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_WARNING), DIAG_ERROR_ALREADY_EXISTS);
    EXPECT_EQ(diag_dtc_register(ctx, 2u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);
    EXPECT_EQ(diag_dtc_register(ctx, 3u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    EXPECT_EQ(diag_dtc_register(ctx, 4u, DIAG_DTC_SEVERITY_CRITICAL), DIAG_ERROR_CAPACITY);
}

TEST_F(DtcFixture, ListsRegisteredDtcsInRegistrationOrder)
{
    std::array<struct diag_dtc_snapshot, 3> out = {};
    size_t count = 0;

    ASSERT_EQ(diag_dtc_register(ctx, 7u, DIAG_DTC_SEVERITY_INFO), DIAG_OK);
    ASSERT_EQ(diag_dtc_register(ctx, 3u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);

    EXPECT_EQ(diag_dtc_list(ctx, out.data(), out.size(), &count), DIAG_OK);
    ASSERT_EQ(count, 2u);
    EXPECT_EQ(out[0].id, 7u);
    EXPECT_EQ(out[1].id, 3u);
}

TEST_F(DtcFixture, ListReportsCapacityWhenOutputIsTooSmall)
{
    std::array<struct diag_dtc_snapshot, 1> out = {};
    size_t count = 0;

    ASSERT_EQ(diag_dtc_register(ctx, 7u, DIAG_DTC_SEVERITY_INFO), DIAG_OK);
    ASSERT_EQ(diag_dtc_register(ctx, 3u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);

    EXPECT_EQ(diag_dtc_list(ctx, out.data(), out.size(), &count), DIAG_ERROR_CAPACITY);
    EXPECT_EQ(count, 2u);
}

TEST_F(DtcFixture, SetActiveIsIdempotentWhileActive)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);

    EXPECT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);
    EXPECT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.active);
    EXPECT_EQ(snapshot.occurrence_count, 1u);
    EXPECT_EQ(snapshot.active_count, 1u);
}

TEST_F(DtcFixture, SetInactivePreservesHistoryAndAllowsNewOccurrence)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);

    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_inactive(ctx, 1u), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.active);
    EXPECT_EQ(snapshot.occurrence_count, 2u);
    EXPECT_EQ(snapshot.active_count, 2u);
    EXPECT_EQ(snapshot.clear_count, 0u);
}

TEST_F(DtcFixture, ClearDeactivatesAndIncrementsClearCountOncePerStateChange)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);

    EXPECT_EQ(diag_dtc_clear(ctx, 1u), DIAG_OK);
    EXPECT_EQ(diag_dtc_clear(ctx, 1u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &snapshot), DIAG_OK);
    EXPECT_FALSE(snapshot.active);
    EXPECT_EQ(snapshot.occurrence_count, 1u);
    EXPECT_EQ(snapshot.active_count, 1u);
    EXPECT_EQ(snapshot.clear_count, 1u);
}

TEST_F(DtcFixture, ClearAllClearsOnlyActiveDtcs)
{
    struct diag_dtc_snapshot first = {};
    struct diag_dtc_snapshot second = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_register(ctx, 2u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);

    EXPECT_EQ(diag_dtc_clear_all(ctx), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &first), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(ctx, 2u, &second), DIAG_OK);
    EXPECT_FALSE(first.active);
    EXPECT_EQ(first.clear_count, 1u);
    EXPECT_EQ(second.clear_count, 0u);
}

TEST_F(DtcFixture, UnknownDtcOperationsReturnNotFound)
{
    struct diag_dtc_snapshot snapshot = {};

    EXPECT_EQ(diag_dtc_get(ctx, 9u, &snapshot), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_set_active(ctx, 9u), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_set_inactive(ctx, 9u), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_clear(ctx, 9u), DIAG_ERROR_NOT_FOUND);
}

TEST_F(DtcFixture, ResetCounterClearsCountersButPreservesRegisteredDtc)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);
    ASSERT_EQ(diag_dtc_clear(ctx, 1u), DIAG_OK);

    EXPECT_EQ(diag_dtc_reset_counter(ctx, 1u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.occurrence_count, 0u);
    EXPECT_EQ(snapshot.active_count, 0u);
    EXPECT_EQ(snapshot.clear_count, 0u);
}

TEST_F(DtcFixture, CountersSaturateAtUint32Max)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    dtc_buffer[0].occurrence_count = UINT32_MAX;
    dtc_buffer[0].active_count = UINT32_MAX;

    EXPECT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.occurrence_count, UINT32_MAX);
    EXPECT_EQ(snapshot.active_count, UINT32_MAX);
}

} // namespace
