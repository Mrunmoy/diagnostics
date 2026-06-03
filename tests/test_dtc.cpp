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
    struct diag_context_storage             storage = {};
    std::array<struct diag_dtc_snapshot, 3> dtc_buffer = {};
    struct diag_context                    *ctx = nullptr;

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
    size_t                   count = 0;

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
    EXPECT_EQ(snapshot.local_fault_id, 0x1234u);
    EXPECT_EQ(snapshot.severity, DIAG_DTC_SEVERITY_CRITICAL);
    EXPECT_FALSE(snapshot.active);
    EXPECT_EQ(snapshot.status, 0u);
    EXPECT_EQ(snapshot.occurrence_count, 0u);
    EXPECT_EQ(snapshot.active_count, 0u);
    EXPECT_EQ(snapshot.clear_count, 0u);
}

TEST_F(DtcFixture, RejectsNullOutputArguments)
{
    struct diag_dtc_snapshot snapshot = {};
    size_t                   count = 0;

    EXPECT_EQ(diag_dtc_get(ctx, 1u, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_get_by_fault(ctx, 1u, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_dtc_get_status(ctx, 1u, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
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

TEST_F(DtcFixture, RegistersLocalFaultMapping)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register_fault(ctx, 0x1001u, 0x0A1234u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 0x0A1234u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.id, 0x0A1234u);
    EXPECT_EQ(snapshot.local_fault_id, 0x1001u);
    EXPECT_EQ(snapshot.severity, DIAG_DTC_SEVERITY_WARNING);

    ASSERT_EQ(diag_dtc_get_by_fault(ctx, 0x1001u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.id, 0x0A1234u);
}

TEST_F(DtcFixture, RejectsDuplicateLocalFaultMappings)
{
    ASSERT_EQ(diag_dtc_register_fault(ctx, 0x1001u, 0x0A1234u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);

    EXPECT_EQ(diag_dtc_register_fault(ctx, 0x1001u, 0x0B1234u, DIAG_DTC_SEVERITY_WARNING),
              DIAG_ERROR_ALREADY_EXISTS);
    EXPECT_EQ(diag_dtc_register_fault(ctx, 0x1002u, 0x0A1234u, DIAG_DTC_SEVERITY_WARNING),
              DIAG_ERROR_ALREADY_EXISTS);
}

TEST_F(DtcFixture, ListsRegisteredDtcsInRegistrationOrder)
{
    std::array<struct diag_dtc_snapshot, 3> out = {};
    size_t                                  count = 0;

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
    size_t                                  count = 0;

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
    EXPECT_EQ(snapshot.status,
              static_cast<uint8_t>(
                  DIAG_DTC_STATUS_TEST_FAILED | DIAG_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE |
                  DIAG_DTC_STATUS_PENDING | DIAG_DTC_STATUS_TEST_FAILED_SINCE_CLEAR));
    EXPECT_EQ(snapshot.occurrence_count, 1u);
    EXPECT_EQ(snapshot.active_count, 1u);
}

TEST_F(DtcFixture, SetFaultTestFailedUpdatesMappedDtcStatus)
{
    struct diag_dtc_snapshot snapshot = {};
    uint8_t                  status = 0u;

    ASSERT_EQ(diag_dtc_register_fault(ctx, 0x1001u, 0x0A1234u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);

    EXPECT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x1001u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get_by_fault(ctx, 0x1001u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.active);
    EXPECT_EQ(snapshot.status,
              static_cast<uint8_t>(
                  DIAG_DTC_STATUS_TEST_FAILED | DIAG_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE |
                  DIAG_DTC_STATUS_PENDING | DIAG_DTC_STATUS_TEST_FAILED_SINCE_CLEAR));
    EXPECT_EQ(snapshot.occurrence_count, 1u);
    EXPECT_EQ(snapshot.active_count, 1u);

    EXPECT_EQ(diag_dtc_get_status(ctx, 0x0A1234u, &status), DIAG_OK);
    EXPECT_EQ(status, snapshot.status);
}

TEST_F(DtcFixture, SetFaultTestPassedClearsCurrentFailureAndPreservesHistory)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register_fault(ctx, 0x1001u, 0x0A1234u, DIAG_DTC_SEVERITY_WARNING), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x1001u), DIAG_OK);

    EXPECT_EQ(diag_dtc_set_fault_test_passed(ctx, 0x1001u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 0x0A1234u, &snapshot), DIAG_OK);
    EXPECT_FALSE(snapshot.active);
    EXPECT_EQ(snapshot.status, static_cast<uint8_t>(DIAG_DTC_STATUS_PENDING |
                                                    DIAG_DTC_STATUS_TEST_FAILED_SINCE_CLEAR));
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
    EXPECT_EQ(snapshot.status, 0u);
    EXPECT_EQ(snapshot.occurrence_count, 1u);
    EXPECT_EQ(snapshot.active_count, 1u);
    EXPECT_EQ(snapshot.clear_count, 1u);
}

TEST_F(DtcFixture, ClearCountsInactiveDtcWhenHistoricalStatusChanges)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_inactive(ctx, 1u), DIAG_OK);

    EXPECT_EQ(diag_dtc_clear(ctx, 1u), DIAG_OK);
    EXPECT_EQ(diag_dtc_clear(ctx, 1u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &snapshot), DIAG_OK);
    EXPECT_FALSE(snapshot.active);
    EXPECT_EQ(snapshot.status, 0u);
    EXPECT_EQ(snapshot.clear_count, 1u);
}

TEST_F(DtcFixture, ClearAllClearsObservableDtcState)
{
    struct diag_dtc_snapshot first = {};
    struct diag_dtc_snapshot second = {};

    ASSERT_EQ(diag_dtc_register(ctx, 1u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_register(ctx, 2u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 1u), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 2u), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_inactive(ctx, 2u), DIAG_OK);

    EXPECT_EQ(diag_dtc_clear_all(ctx), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 1u, &first), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(ctx, 2u, &second), DIAG_OK);
    EXPECT_FALSE(first.active);
    EXPECT_EQ(first.status, 0u);
    EXPECT_EQ(first.clear_count, 1u);
    EXPECT_FALSE(second.active);
    EXPECT_EQ(second.status, 0u);
    EXPECT_EQ(second.clear_count, 1u);
}

TEST_F(DtcFixture, UnknownDtcOperationsReturnNotFound)
{
    struct diag_dtc_snapshot snapshot = {};

    EXPECT_EQ(diag_dtc_get(ctx, 9u, &snapshot), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_set_active(ctx, 9u), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_set_inactive(ctx, 9u), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_set_fault_test_failed(ctx, 9u), DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(diag_dtc_set_fault_test_passed(ctx, 9u), DIAG_ERROR_NOT_FOUND);
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

// ── Operation-cycle lifecycle ───────────────────────────────────────────────

// Builds a context with explicit confirmation/aging thresholds.
struct CycleContext
{
    struct diag_context_storage             storage = {};
    std::array<struct diag_dtc_snapshot, 3> buffer = {};
    struct diag_context                    *ctx = nullptr;

    CycleContext(uint8_t confirmation_threshold, uint16_t aging_threshold)
    {
        struct diag_config config = {};
        config.dtc_buffer = buffer.data();
        config.dtc_capacity = buffer.size();
        config.dtc.confirmation_threshold = confirmation_threshold;
        config.dtc.aging_threshold = aging_threshold;
        EXPECT_EQ(diag_init(&storage, &config, &ctx), DIAG_OK);
    }
};

TEST(DiagDtcOperationCycle, RejectsNullContext)
{
    EXPECT_EQ(diag_dtc_operation_cycle(nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST_F(DtcFixture, OperationCycleConfirmsAfterDefaultThreshold)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 0x100u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x100u), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 0x100u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_PENDING);
    EXPECT_FALSE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE);

    // Default confirmation threshold is a single failed operation cycle.
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 0x100u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
    EXPECT_FALSE(snapshot.status & DIAG_DTC_STATUS_TEST_FAILED_THIS_OPERATION_CYCLE);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE);
    EXPECT_FALSE(snapshot.failed_this_cycle);
}

TEST_F(DtcFixture, OperationCycleClearsPendingAfterCleanCycle)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 0x101u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x101u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 0x101u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_PENDING);

    // A clean cycle (no failure reported) clears the pending bit.
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);

    ASSERT_EQ(diag_dtc_get(ctx, 0x101u, &snapshot), DIAG_OK);
    EXPECT_FALSE(snapshot.status & DIAG_DTC_STATUS_PENDING);
}

TEST(DiagDtcOperationCycle, ConfirmsOnlyAfterConfiguredThreshold)
{
    CycleContext c(3u, 0u);
    ASSERT_NE(c.ctx, nullptr);
    ASSERT_EQ(diag_dtc_register(c.ctx, 0x200u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);

    struct diag_dtc_snapshot snapshot = {};
    for (int cycle = 1; cycle <= 2; ++cycle)
    {
        ASSERT_EQ(diag_dtc_set_fault_test_failed(c.ctx, 0x200u), DIAG_OK);
        ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
        ASSERT_EQ(diag_dtc_get(c.ctx, 0x200u, &snapshot), DIAG_OK);
        EXPECT_FALSE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED) << "cycle " << cycle;
    }

    ASSERT_EQ(diag_dtc_set_fault_test_failed(c.ctx, 0x200u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(c.ctx, 0x200u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
}

TEST_F(DtcFixture, OperationCyclePendingAndConfirmedCoexistThenPendingClears)
{
    struct diag_dtc_snapshot snapshot = {};

    ASSERT_EQ(diag_dtc_register(ctx, 0x150u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x150u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);

    // Per ISO 14229, pendingDTC and confirmedDTC are not mutually exclusive: a
    // confirmed DTC still failing this cycle is both.
    ASSERT_EQ(diag_dtc_get(ctx, 0x150u, &snapshot), DIAG_OK);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_PENDING);

    // A clean cycle clears PENDING but keeps CONFIRMED.
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(ctx, 0x150u, &snapshot), DIAG_OK);
    EXPECT_FALSE(snapshot.status & DIAG_DTC_STATUS_PENDING);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
}

TEST(DiagDtcOperationCycle, AgesOutConfirmedDtcAfterCleanCycles)
{
    CycleContext c(1u, 3u);
    ASSERT_NE(c.ctx, nullptr);
    ASSERT_EQ(diag_dtc_register(c.ctx, 0x300u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);

    struct diag_dtc_snapshot snapshot = {};
    ASSERT_EQ(diag_dtc_set_fault_test_failed(c.ctx, 0x300u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(c.ctx, 0x300u, &snapshot), DIAG_OK);
    ASSERT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);

    for (int i = 0; i < 2; ++i)
    {
        ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
        ASSERT_EQ(diag_dtc_get(c.ctx, 0x300u, &snapshot), DIAG_OK);
        EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED) << "clean cycle " << i;
    }

    // Third consecutive clean cycle reaches the aging threshold and heals the DTC.
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(c.ctx, 0x300u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.status, 0u);
    EXPECT_FALSE(snapshot.active);
}

TEST(DiagDtcOperationCycle, AgingResetsWhenFaultReoccurs)
{
    CycleContext c(1u, 5u);
    ASSERT_NE(c.ctx, nullptr);
    ASSERT_EQ(diag_dtc_register(c.ctx, 0x400u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);

    struct diag_dtc_snapshot snapshot = {};
    ASSERT_EQ(diag_dtc_set_fault_test_failed(c.ctx, 0x400u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(c.ctx, 0x400u, &snapshot), DIAG_OK);
    ASSERT_EQ(snapshot.aging_counter, 2u);

    ASSERT_EQ(diag_dtc_set_fault_test_failed(c.ctx, 0x400u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(c.ctx), DIAG_OK);
    ASSERT_EQ(diag_dtc_get(c.ctx, 0x400u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.aging_counter, 0u);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
}

TEST_F(DtcFixture, ClearResetsOperationCycleState)
{
    ASSERT_EQ(diag_dtc_register(ctx, 0x500u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x500u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);

    ASSERT_EQ(diag_dtc_clear(ctx, 0x500u), DIAG_OK);

    struct diag_dtc_snapshot snapshot = {};
    ASSERT_EQ(diag_dtc_get(ctx, 0x500u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.status, 0u);
    EXPECT_FALSE(snapshot.failed_this_cycle);
    EXPECT_EQ(snapshot.failed_cycle_count, 0u);
    EXPECT_EQ(snapshot.aging_counter, 0u);
}

} // namespace
