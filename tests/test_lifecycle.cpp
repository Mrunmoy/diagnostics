#include "diag/diag.hpp"

#include <gtest/gtest.h>
#include <limits>

namespace
{

class LifecycleFixture : public testing::Test
{
  protected:
    diag::ContextStorage m_storage{};
    diag::Context        m_context{m_storage};
};

} // namespace

TEST_F(LifecycleFixture, ReportsNotFoundBeforeLifecycleAttach)
{
    EXPECT_EQ(m_context.lifecycle().result(), diag::Result::NotFound);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::PowerOn), diag::Result::NotFound);
    EXPECT_EQ(m_context.clearLifecycleDirty(
                  static_cast<diag::LifecycleDirtyFlags>(diag::LifecycleDirtyFlag::ResetCounter)),
              diag::Result::NotFound);
}

TEST_F(LifecycleFixture, DisabledPolicyTracksLastReasonWithoutCountingOrDirtying)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::Disabled, 0U, 0U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Watchdog), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().lastResetReason, diag::ResetReason::Watchdog);
    EXPECT_EQ(snapshot.value().resetCount, 0U);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 0U);
    EXPECT_EQ(snapshot.value().dirtyFlags, 0U);
    EXPECT_FALSE(snapshot.value().persistRequested);
    EXPECT_EQ(m_context.dirtyFlags(), 0U);
}

TEST_F(LifecycleFixture, RamOnlyPolicyCountsWithoutRequestingPersistence)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::RamOnly, 0U, 0U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::PowerOn), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Brownout), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().resetCounterPolicy, diag::ResetCounterPolicy::RamOnly);
    EXPECT_EQ(snapshot.value().resetCount, 2U);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 1U);
    EXPECT_FALSE(snapshot.value().persistRequested);
    EXPECT_EQ(m_context.dirtyFlags(), 0U);
}

TEST_F(LifecycleFixture, AbnormalOnlyPolicyMarksDirtyOnlyForAbnormalReset)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Software), diag::Result::Ok);
    EXPECT_EQ(m_context.dirtyFlags(), 0U);

    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Fault), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().resetCount, 2U);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 1U);
    EXPECT_EQ(snapshot.value().dirtyFlags,
              static_cast<diag::LifecycleDirtyFlags>(diag::LifecycleDirtyFlag::ResetCounter));
    EXPECT_TRUE(snapshot.value().persistRequested);
    EXPECT_EQ(m_context.dirtyFlags(), static_cast<diag::DirtyFlags>(diag::DirtyFlag::Lifecycle));
}

TEST_F(LifecycleFixture, EveryNPolicyRequestsPersistenceOnlyAtConfiguredInterval)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::EveryN, 3U, 0U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::PowerOn), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::External), diag::Result::Ok);
    EXPECT_EQ(m_context.dirtyFlags(), 0U);

    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Software), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().resetCount, 3U);
    EXPECT_TRUE(snapshot.value().persistRequested);
    EXPECT_EQ(m_context.dirtyFlags(), static_cast<diag::DirtyFlags>(diag::DirtyFlag::Lifecycle));
}

TEST_F(LifecycleFixture, EveryNPolicyWithZeroIntervalNeverRequestsPersistence)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::EveryN, 0U, 0U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::PowerOn), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().resetCount, 1U);
    EXPECT_FALSE(snapshot.value().persistRequested);
    EXPECT_EQ(m_context.dirtyFlags(), 0U);
}

TEST_F(LifecycleFixture, PlatformPolicyUsesConfiguredCounterWithoutIncrementing)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::Platform, 0U, 42U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Watchdog), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().resetCount, 42U);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 0U);
    EXPECT_FALSE(snapshot.value().persistRequested);
}

TEST_F(LifecycleFixture, ClearDirtyResetsLifecycleAndContextDirtyFlags)
{
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};

    EXPECT_EQ(m_context.attachLifecycle(config), diag::Result::Ok);
    EXPECT_EQ(m_context.observeReset(diag::ResetReason::Brownout), diag::Result::Ok);
    EXPECT_EQ(m_context.clearLifecycleDirty(
                  static_cast<diag::LifecycleDirtyFlags>(diag::LifecycleDirtyFlag::ResetCounter)),
              diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = m_context.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().dirtyFlags, 0U);
    EXPECT_FALSE(snapshot.value().persistRequested);
    EXPECT_EQ(m_context.dirtyFlags(), 0U);
}
