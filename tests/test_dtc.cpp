#include "diag/diag.hpp"

#include <gtest/gtest.h>
#include <limits>

namespace
{

class DtcFixture : public testing::Test
{
  protected:
    diag::DtcRecord      m_records[2]{};
    diag::Config         m_config{m_records, 2U};
    diag::ContextStorage m_storage{};
    diag::Context        m_context{m_storage, m_config};
};

} // namespace

TEST(DiagDtc, RecordLayoutMatchesCapsuleEncoding)
{
    EXPECT_EQ(diag::DtcRecord::kEncodedSize, 28U);
    EXPECT_EQ(sizeof(diag::DtcRecord), diag::DtcRecord::kEncodedSize);
}

TEST(DiagDtc, RejectsRegistrationWithoutCallerOwnedBuffer)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};

    EXPECT_EQ(context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Warning),
              diag::Result::InvalidArgument);
}

TEST_F(DtcFixture, RegistersAndReadsDtc)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{0x040101U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.dtcCount(), 1U);
    EXPECT_EQ(m_context.dirtyFlags(), static_cast<diag::DirtyFlags>(diag::DirtyFlag::Dtc));

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{0x040101U});

    ASSERT_TRUE(record.hasValue());
    EXPECT_EQ(record.value().id, diag::DtcId{0x040101U});
    EXPECT_EQ(record.value().localFaultId, diag::LocalFaultId{0x040101U});
    EXPECT_EQ(record.value().severity, diag::DtcSeverity::Critical);
    EXPECT_EQ(record.value().status, 0U);
    EXPECT_FALSE(record.value().active);
}

TEST_F(DtcFixture, RegistersFaultMapping)
{
    EXPECT_EQ(m_context.registerFault(diag::LocalFaultId{7U}, diag::DtcId{0x040101U},
                                      diag::DtcSeverity::Error),
              diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> byFault = m_context.dtcByFault(diag::LocalFaultId{7U});
    ASSERT_TRUE(byFault.hasValue());
    EXPECT_EQ(byFault.value().id, diag::DtcId{0x040101U});
    EXPECT_EQ(byFault.value().severity, diag::DtcSeverity::Error);
}

TEST_F(DtcFixture, RejectsDuplicateAndCapacityOverflow)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Info), diag::Result::Ok);
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Warning),
              diag::Result::AlreadyExists);
    EXPECT_EQ(m_context.registerFault(diag::LocalFaultId{1U}, diag::DtcId{9U},
                                      diag::DtcSeverity::Warning),
              diag::Result::AlreadyExists);
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{2U}, diag::DtcSeverity::Warning), diag::Result::Ok);
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{3U}, diag::DtcSeverity::Critical),
              diag::Result::Capacity);
}

TEST_F(DtcFixture, ListsRegisteredDtcsInRegistrationOrder)
{
    diag::DtcRecord listed[2]{};
    std::size_t     count{0U};

    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Info), diag::Result::Ok);
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{2U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);

    EXPECT_EQ(m_context.listDtcs(listed, 2U, count), diag::Result::Ok);
    EXPECT_EQ(count, 2U);
    EXPECT_EQ(listed[0].id, diag::DtcId{1U});
    EXPECT_EQ(listed[1].id, diag::DtcId{2U});
}

TEST_F(DtcFixture, ListReportsRequiredCountWhenOutputIsTooSmall)
{
    diag::DtcRecord listed[1]{};
    std::size_t     count{0U};

    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Info), diag::Result::Ok);
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{2U}, diag::DtcSeverity::Warning), diag::Result::Ok);

    EXPECT_EQ(m_context.listDtcs(listed, 1U, count), diag::Result::Capacity);
    EXPECT_EQ(count, 2U);
}

TEST_F(DtcFixture, SetActiveMarksFailedWithoutInstantConfirm)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.clearDirty(diag::DirtyFlag::Dtc), diag::Result::Ok);

    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_TRUE(record.value().active);
    EXPECT_TRUE(record.value().failedThisCycle);
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailed));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Pending));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailedThisCycle));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailedSinceClear));
    EXPECT_FALSE(diag::hasStatus(record.value(), diag::DtcStatus::Confirmed));
    EXPECT_EQ(record.value().occurrenceCount, 1U);
    EXPECT_EQ(record.value().activeCount, 1U);
    EXPECT_EQ(m_context.dirtyFlags(), static_cast<diag::DirtyFlags>(diag::DirtyFlag::Dtc));
}

TEST_F(DtcFixture, OperationCycleConfirmsAfterFailedCycle)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_context.operationCycle(), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Confirmed));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Pending));
    EXPECT_FALSE(record.value().failedThisCycle);
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::TestNotCompletedThisCycle));
}

TEST_F(DtcFixture, SetInactivePreservesPendingUntilCleanCycle)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, false), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_FALSE(record.value().active);
    EXPECT_FALSE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailed));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Pending));
    EXPECT_EQ(record.value().occurrenceCount, 1U);
}

TEST_F(DtcFixture, AgingClearsConfirmedAfterThresholdCleanCycles)
{
    diag::Config         config{m_records, 2U, 1U, 2U};
    diag::ContextStorage storage{};
    diag::Context        context{storage, config};

    ASSERT_EQ(context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Warning), diag::Result::Ok);
    ASSERT_EQ(context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    ASSERT_EQ(context.operationCycle(), diag::Result::Ok);
    ASSERT_EQ(context.setDtcActive(diag::DtcId{1U}, false), diag::Result::Ok);
    ASSERT_EQ(context.operationCycle(), diag::Result::Ok);
    ASSERT_EQ(context.operationCycle(), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_EQ(record.value().status, 0U);
    EXPECT_EQ(record.value().agingCounter, 0U);
}

TEST_F(DtcFixture, ClearDtcClearsStatusAndKeepsOccurrenceCount)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);

    EXPECT_EQ(m_context.clearDtc(diag::DtcId{1U}), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_EQ(record.value().status, 0U);
    EXPECT_EQ(record.value().occurrenceCount, 1U);
    EXPECT_EQ(record.value().clearCount, 1U);
    EXPECT_FALSE(record.value().active);
}

TEST_F(DtcFixture, ClearAllAndResetCounter)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_context.clearAllDtcs(), diag::Result::Ok);
    EXPECT_EQ(m_records[0].status, 0U);
    EXPECT_EQ(m_records[0].clearCount, 1U);

    EXPECT_EQ(m_context.resetDtcCounter(diag::DtcId{1U}), diag::Result::Ok);
    EXPECT_EQ(m_records[0].occurrenceCount, 0U);
    EXPECT_EQ(m_records[0].activeCount, 0U);
    EXPECT_EQ(m_records[0].clearCount, 0U);
}

TEST_F(DtcFixture, FaultApisAndStatusReadback)
{
    EXPECT_EQ(m_context.registerFault(diag::LocalFaultId{42U}, diag::DtcId{1U},
                                      diag::DtcSeverity::Warning),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setFaultTestFailed(diag::LocalFaultId{42U}), diag::Result::Ok);

    const diag::ResultValue<diag::DtcStatusFlags> status = m_context.dtcStatus(diag::DtcId{1U});
    ASSERT_TRUE(status.hasValue());
    EXPECT_NE((status.value() & static_cast<diag::DtcStatusFlags>(diag::DtcStatus::TestFailed)),
              0U);

    EXPECT_EQ(m_context.setFaultTestPassed(diag::LocalFaultId{42U}), diag::Result::Ok);
    EXPECT_FALSE(m_records[0].active);
}

TEST_F(DtcFixture, CountersSaturateAtUint32Max)
{
    constexpr std::uint32_t maxCounter = std::numeric_limits<std::uint32_t>::max();

    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);

    m_records[0].occurrenceCount = maxCounter;
    m_records[0].activeCount = maxCounter;
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_records[0].occurrenceCount, maxCounter);
    EXPECT_EQ(m_records[0].activeCount, maxCounter);

    m_records[0].status = static_cast<diag::DtcStatusFlags>(diag::DtcStatus::Confirmed);
    m_records[0].clearCount = maxCounter;
    EXPECT_EQ(m_context.clearDtc(diag::DtcId{1U}), diag::Result::Ok);
    EXPECT_EQ(m_records[0].clearCount, maxCounter);
}

TEST_F(DtcFixture, UnknownDtcOperationsReturnNotFound)
{
    EXPECT_EQ(m_context.dtc(diag::DtcId{99U}).result(), diag::Result::NotFound);
    EXPECT_EQ(m_context.dtcByFault(diag::LocalFaultId{99U}).result(), diag::Result::NotFound);
    EXPECT_EQ(m_context.dtcStatus(diag::DtcId{99U}).result(), diag::Result::NotFound);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{99U}, true), diag::Result::NotFound);
    EXPECT_EQ(m_context.clearDtc(diag::DtcId{99U}), diag::Result::NotFound);
}
