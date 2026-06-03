#include <gtest/gtest.h>

extern "C"
{
#include "diag/diag.h"
}

namespace
{

/** Test harness for reset-policy cases that only need one DTC slot to initialize context. */
struct LifecycleFixture
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
    struct diag_dtc_snapshot    dtc_buffer[1] = {};
};

struct diag_config make_config(LifecycleFixture &fixture, enum diag_reset_counter_policy policy,
                               uint32_t every_n)
{
    struct diag_config config = {
        /* dtc_buffer       */ fixture.dtc_buffer,
        /* dtc_capacity     */ 1,
        /* storage          */ {},
        /* transport        */ {},
        /* identity         */ {},
        /* lifecycle        */
        {
            /* reset_counter_policy */ policy,
            /* reset_count_interval */ every_n,
            /* platform_reset_count */ 0,
        },
    };

    return config;
}

TEST(DiagLifecycle, RejectsNullArguments)
{
    struct diag_lifecycle_snapshot snapshot = {};

    EXPECT_EQ(diag_lifecycle_observe_reset(nullptr, DIAG_RESET_REASON_POWER_ON),
              DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_lifecycle_get(nullptr, &snapshot), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_lifecycle_get(nullptr, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_lifecycle_clear_dirty(nullptr, DIAG_LIFECYCLE_DIRTY_RESET_COUNTER),
              DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagLifecycle, RejectsNullSnapshotWithoutChangingLifecycleState)
{
    LifecycleFixture         fixture;
    const struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_RAM_ONLY, 0);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_POWER_ON), DIAG_OK);

    EXPECT_EQ(diag_lifecycle_get(fixture.ctx, nullptr), DIAG_ERROR_INVALID_ARGUMENT);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.reset_count, 1u);
    EXPECT_EQ(snapshot.last_reset_reason, DIAG_RESET_REASON_POWER_ON);
}

TEST(DiagLifecycle, DisabledPolicyTracksReasonWithoutDirtyStorage)
{
    LifecycleFixture         fixture;
    const struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_DISABLED, 0);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_POWER_ON), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.last_reset_reason, DIAG_RESET_REASON_POWER_ON);
    EXPECT_EQ(snapshot.reset_count, 0u);
    EXPECT_EQ(snapshot.abnormal_reset_count, 0u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);

    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_WATCHDOG), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.last_reset_reason, DIAG_RESET_REASON_WATCHDOG);
    EXPECT_EQ(snapshot.reset_count, 0u);
    EXPECT_EQ(snapshot.abnormal_reset_count, 0u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);
}

TEST(DiagLifecycle, RamOnlyPolicyCountsWithoutDirtyStorage)
{
    LifecycleFixture         fixture;
    const struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_RAM_ONLY, 0);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_POWER_ON), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_SOFTWARE), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.reset_count, 2u);
    EXPECT_EQ(snapshot.abnormal_reset_count, 0u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);
}

TEST(DiagLifecycle, AbnormalOnlyPolicyMarksDirtyForAbnormalReset)
{
    LifecycleFixture         fixture;
    const struct diag_config config =
        make_config(fixture, DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY, 0);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_POWER_ON), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);

    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_WATCHDOG), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.reset_count, 2u);
    EXPECT_EQ(snapshot.abnormal_reset_count, 1u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_RESET_COUNTER);
    EXPECT_TRUE(snapshot.persist_requested);
}

TEST(DiagLifecycle, EveryNPolicyMarksDirtyOnlyAtInterval)
{
    LifecycleFixture         fixture;
    const struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_EVERY_N, 3);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_POWER_ON), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);

    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_SOFTWARE), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);

    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_EXTERNAL), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.reset_count, 3u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_RESET_COUNTER);
    EXPECT_TRUE(snapshot.persist_requested);
}

TEST(DiagLifecycle, ClearDirtyResetsPersistenceRequest)
{
    LifecycleFixture         fixture;
    const struct diag_config config =
        make_config(fixture, DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY, 0);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_FAULT), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_clear_dirty(fixture.ctx, DIAG_LIFECYCLE_DIRTY_RESET_COUNTER), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);
}

TEST(DiagLifecycle, PlatformPolicyUsesConfiguredPlatformCounter)
{
    LifecycleFixture   fixture;
    struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_PLATFORM, 0);
    config.lifecycle.platform_reset_count = 42;

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_SOFTWARE), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.reset_count, 42u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);
}

TEST(DiagLifecycle, PlatformPolicyDoesNotCountAbnormalResetsInLibrary)
{
    LifecycleFixture   fixture;
    struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_PLATFORM, 0);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(fixture.ctx, DIAG_RESET_REASON_WATCHDOG), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.last_reset_reason, DIAG_RESET_REASON_WATCHDOG);
    EXPECT_EQ(snapshot.reset_count, 0u);
    EXPECT_EQ(snapshot.abnormal_reset_count, 0u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_FALSE(snapshot.persist_requested);
}

TEST(DiagLifecycle, NonPlatformPolicyIgnoresConfiguredPlatformCounter)
{
    LifecycleFixture   fixture;
    struct diag_config config = make_config(fixture, DIAG_RESET_COUNTER_POLICY_RAM_ONLY, 0);
    config.lifecycle.platform_reset_count = 42;

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(fixture.ctx, &snapshot), DIAG_OK);

    EXPECT_EQ(snapshot.reset_count, 0u);
}

} // namespace
