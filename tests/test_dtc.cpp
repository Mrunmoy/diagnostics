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
    EXPECT_EQ(record.value().severity, diag::DtcSeverity::Critical);
    EXPECT_EQ(record.value().status, 0U);
}

TEST_F(DtcFixture, RejectsDuplicateAndCapacityOverflow)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Info), diag::Result::Ok);
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Warning),
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

TEST_F(DtcFixture, SetActiveUpdatesStatusAndCountsOnlyStateChanges)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.clearDirty(diag::DirtyFlag::Dtc), diag::Result::Ok);

    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailed));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Pending));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Confirmed));
    EXPECT_EQ(record.value().occurrenceCount, 1U);
    EXPECT_EQ(m_context.dirtyFlags(), static_cast<diag::DirtyFlags>(diag::DirtyFlag::Dtc));
}

TEST_F(DtcFixture, SetInactivePreservesHistory)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, false), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_FALSE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailed));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Pending));
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::Confirmed));
    EXPECT_EQ(record.value().occurrenceCount, 1U);
}

TEST_F(DtcFixture, ClearDtcClearsStatusAndIncrementsClearCount)
{
    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);

    EXPECT_EQ(m_context.clearDtc(diag::DtcId{1U}), diag::Result::Ok);

    const diag::ResultValue<diag::DtcRecord> record = m_context.dtc(diag::DtcId{1U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_EQ(record.value().status, 0U);
    EXPECT_EQ(record.value().occurrenceCount, 0U);
    EXPECT_EQ(record.value().clearCount, 1U);
}

TEST_F(DtcFixture, CountersSaturateAtUint32Max)
{
    constexpr std::uint32_t maxCounter = std::numeric_limits<std::uint32_t>::max();

    EXPECT_EQ(m_context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);

    m_records[0].occurrenceCount = maxCounter;
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{1U}, true), diag::Result::Ok);
    EXPECT_EQ(m_records[0].occurrenceCount, maxCounter);

    m_records[0].status = static_cast<diag::DtcStatusFlags>(diag::DtcStatus::Confirmed);
    m_records[0].clearCount = maxCounter;
    EXPECT_EQ(m_context.clearDtc(diag::DtcId{1U}), diag::Result::Ok);
    EXPECT_EQ(m_records[0].clearCount, maxCounter);
}

TEST_F(DtcFixture, UnknownDtcOperationsReturnNotFound)
{
    EXPECT_EQ(m_context.dtc(diag::DtcId{99U}).result(), diag::Result::NotFound);
    EXPECT_EQ(m_context.setDtcActive(diag::DtcId{99U}, true), diag::Result::NotFound);
    EXPECT_EQ(m_context.clearDtc(diag::DtcId{99U}), diag::Result::NotFound);
}
