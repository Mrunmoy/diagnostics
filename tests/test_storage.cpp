#include "diag/capsule.hpp"
#include "diag/context.hpp"
#include "diag/storage.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

namespace
{

constexpr std::size_t kStorageBytes = 256U;

struct MemoryStorage
{
    std::array<std::uint8_t, kStorageBytes> persisted{};
    std::size_t                             persistedLength{0U};
    std::size_t                             loadCalls{0U};
    std::size_t                             saveCalls{0U};
    std::size_t                             clearCalls{0U};
};

diag::Result memoryLoad(void *const user, std::uint8_t *const buffer, const std::size_t capacity,
                        std::size_t &bytesRead) noexcept
{
    MemoryStorage &storage = *static_cast<MemoryStorage *>(user);
    ++storage.loadCalls;
    bytesRead = 0U;

    if (capacity < storage.persistedLength)
    {
        return diag::Result::Capacity;
    }

    for (std::size_t index = 0U; index < storage.persistedLength; ++index)
    {
        buffer[index] = storage.persisted[index];
    }

    bytesRead = storage.persistedLength;
    return diag::Result::Ok;
}

diag::Result memorySave(void *const user, const std::uint8_t *const buffer,
                        const std::size_t length) noexcept
{
    MemoryStorage &storage = *static_cast<MemoryStorage *>(user);
    ++storage.saveCalls;

    if (length > storage.persisted.size())
    {
        return diag::Result::Capacity;
    }

    for (std::size_t index = 0U; index < length; ++index)
    {
        storage.persisted[index] = buffer[index];
    }

    storage.persistedLength = length;
    return diag::Result::Ok;
}

diag::Result memoryClear(void *const user) noexcept
{
    MemoryStorage &storage = *static_cast<MemoryStorage *>(user);
    ++storage.clearCalls;
    storage.persistedLength = 0U;
    return diag::Result::Ok;
}

diag::Result oversizedLoad(void *, std::uint8_t *, const std::size_t capacity,
                           std::size_t &bytesRead) noexcept
{
    bytesRead = capacity + 1U;
    return diag::Result::Ok;
}

void writeU32Le(std::array<std::uint8_t, kStorageBytes> &buffer, const std::size_t offset,
                const std::uint32_t value)
{
    buffer[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    buffer[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buffer[offset + 2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    buffer[offset + 3U] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

class StorageFixture : public testing::Test
{
  protected:
    diag::Storage makeStorage()
    {
        return diag::Storage{
            diag::StorageOps{memoryLoad, memorySave, memoryClear},
            &m_storage,
            diag::StorageCapabilities{0xFFU, 8U, diag::StorageAtomicCommit::Adapter,
                                      diag::StorageWearLeveling::Adapter},
            m_capsuleBuffer.data(),
            m_capsuleBuffer.size(),
        };
    }

    std::array<std::uint8_t, kStorageBytes> m_capsuleBuffer{};
    MemoryStorage                           m_storage{};
};

} // namespace

TEST(DiagStorage, ValidatesCallbacksAndCapabilities)
{
    MemoryStorage                           memory{};
    std::array<std::uint8_t, kStorageBytes> capsule{};

    diag::Storage storage{diag::StorageOps{memoryLoad, memorySave, memoryClear},
                          &memory,
                          {},
                          capsule.data(),
                          capsule.size()};
    EXPECT_EQ(diag::validateStorage(storage), diag::Result::Ok);

    storage.ops.load = nullptr;
    EXPECT_EQ(diag::validateStorage(storage), diag::Result::InvalidArgument);

    storage.ops.load = memoryLoad;
    storage.capabilities.writeAlignment = 0U;
    EXPECT_EQ(diag::validateStorage(storage), diag::Result::InvalidArgument);
}

TEST(DiagStorage, LoadRejectsAdapterReportedOverflow)
{
    MemoryStorage                           memory{};
    std::array<std::uint8_t, kStorageBytes> buffer{};
    std::size_t                             bytesRead = 7U;
    const diag::Storage                     storage{
        diag::StorageOps{oversizedLoad, memorySave, memoryClear}, &memory, {}, nullptr, 0U};

    EXPECT_EQ(diag::storageLoad(storage, buffer.data(), buffer.size(), bytesRead),
              diag::Result::Storage);
    EXPECT_EQ(bytesRead, 0U);
}

TEST_F(StorageFixture, SaveCleanContextDoesNotCallAdapter)
{
    diag::ContextStorage contextStorage{};
    diag::Context        context{contextStorage};

    ASSERT_EQ(context.attachStorage(makeStorage()), diag::Result::Ok);

    EXPECT_EQ(context.savePersistent(), diag::Result::Ok);
    EXPECT_EQ(m_storage.saveCalls, 0U);
    EXPECT_EQ(m_storage.persistedLength, 0U);
}

TEST_F(StorageFixture, PersistenceOperationsReportMissingStorageWhenNotAttached)
{
    diag::ContextStorage contextStorage{};
    diag::Context        context{contextStorage};

    EXPECT_EQ(context.savePersistent(), diag::Result::NotFound);
    EXPECT_EQ(context.loadPersistent(), diag::Result::NotFound);
    EXPECT_EQ(context.clearPersistent(), diag::Result::NotFound);
}

TEST_F(StorageFixture, SaveDirtyDtcWritesCapsuleAndClearsDirty)
{
    constexpr std::uint8_t kUntouchedByte = 0xA5U;

    std::array<diag::DtcRecord, 2U> records{};
    diag::ContextStorage            contextStorage{};
    diag::Context context{contextStorage, diag::Config{records.data(), records.size()}};

    m_capsuleBuffer.fill(kUntouchedByte);

    ASSERT_EQ(context.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(context.registerDtc(diag::DtcId{0x010203U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    ASSERT_EQ(context.setDtcActive(diag::DtcId{0x010203U}, true), diag::Result::Ok);

    EXPECT_EQ(context.savePersistent(), diag::Result::Ok);

    EXPECT_EQ(m_storage.saveCalls, 1U);
    EXPECT_EQ(context.dirtyFlags(), diag::DirtyFlags{0U});
    EXPECT_NE(m_storage.persistedLength, 0U);
    EXPECT_EQ(m_storage.persistedLength % 8U, 0U);

    const diag::ResultValue<diag::CapsuleDescriptor> descriptor =
        diag::decodeCapsule(m_storage.persisted.data(), m_storage.persistedLength);
    ASSERT_TRUE(descriptor.hasValue());

    const diag::ResultValue<diag::CapsuleSection> section = diag::findCapsuleSectionByType(
        descriptor.value(), static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc));
    ASSERT_TRUE(section.hasValue());
    EXPECT_EQ(section.value().usedLength, 20U);

    for (std::size_t index = m_storage.persistedLength; index < m_capsuleBuffer.size(); ++index)
    {
        EXPECT_EQ(m_capsuleBuffer[index], kUntouchedByte);
    }
}

TEST_F(StorageFixture, LoadSavedDtcCapsuleRestoresRecords)
{
    std::array<diag::DtcRecord, 2U> sourceRecords{};
    diag::ContextStorage            sourceStorage{};
    diag::Context source{sourceStorage, diag::Config{sourceRecords.data(), sourceRecords.size()}};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.registerDtc(diag::DtcId{0x040506U}, diag::DtcSeverity::Warning),
              diag::Result::Ok);
    ASSERT_EQ(source.setDtcActive(diag::DtcId{0x040506U}, true), diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    std::array<diag::DtcRecord, 2U>         restoredRecords{};
    diag::ContextStorage                    restoredStorage{};
    diag::Config                            restoredConfig{};
    restoredConfig.dtcRecords = restoredRecords.data();
    restoredConfig.dtcCapacity = restoredRecords.size();
    diag::Context restored{restoredStorage, restoredConfig};
    diag::Storage restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);

    EXPECT_EQ(restored.loadPersistent(), diag::Result::Ok);
    EXPECT_EQ(restored.dirtyFlags(), diag::DirtyFlags{0U});
    EXPECT_EQ(restored.dtcCount(), 1U);

    const diag::ResultValue<diag::DtcRecord> record = restored.dtc(diag::DtcId{0x040506U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_EQ(record.value().severity, diag::DtcSeverity::Warning);
    EXPECT_TRUE(diag::hasStatus(record.value(), diag::DtcStatus::TestFailed));
    EXPECT_EQ(record.value().occurrenceCount, 1U);
}

TEST_F(StorageFixture, LoadRejectsDtcPayloadWithUnknownStatusBits)
{
    std::array<diag::DtcRecord, 2U> sourceRecords{};
    diag::ContextStorage            sourceStorage{};
    diag::Context source{sourceStorage, diag::Config{sourceRecords.data(), sourceRecords.size()}};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.registerDtc(diag::DtcId{0x070809U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    ASSERT_EQ(source.setDtcActive(diag::DtcId{0x070809U}, true), diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    const diag::ResultValue<diag::CapsuleDescriptor> descriptor =
        diag::decodeCapsule(m_storage.persisted.data(), m_storage.persistedLength);
    ASSERT_TRUE(descriptor.hasValue());

    const diag::ResultValue<diag::CapsuleSection> section = diag::findCapsuleSectionByType(
        descriptor.value(), static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc));
    ASSERT_TRUE(section.hasValue());

    constexpr std::uint8_t kUnknownStatusBit = 0x80U;
    m_storage.persisted[section.value().offset + 16U] |= kUnknownStatusBit;

    const diag::ResultValue<std::uint32_t> crc =
        diag::capsuleCrc32(&m_storage.persisted[diag::kCapsuleHeaderSize],
                           m_storage.persistedLength - diag::kCapsuleHeaderSize);
    ASSERT_TRUE(crc.hasValue());
    writeU32Le(m_storage.persisted, diag::kCapsuleHeaderSize - 4U, crc.value());

    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    std::array<diag::DtcRecord, 2U>         restoredRecords{};
    diag::ContextStorage                    restoredStorage{};
    diag::Config                            restoredConfig{};
    restoredConfig.dtcRecords = restoredRecords.data();
    restoredConfig.dtcCapacity = restoredRecords.size();
    diag::Context restored{restoredStorage, restoredConfig};
    diag::Storage restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);

    EXPECT_EQ(restored.loadPersistent(), diag::Result::CorruptData);
    EXPECT_EQ(restored.dtcCount(), 0U);
}

TEST_F(StorageFixture, SaveAndLoadLifecycleCounters)
{
    diag::ContextStorage        sourceStorage{};
    diag::Context               source{sourceStorage};
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.attachLifecycle(config), diag::Result::Ok);
    ASSERT_EQ(source.observeReset(diag::ResetReason::Watchdog), diag::Result::Ok);
    ASSERT_NE(source.dirtyFlags(), diag::DirtyFlags{0U});
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    diag::ContextStorage                    restoredStorage{};
    diag::Context                           restored{restoredStorage};
    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    diag::Storage                           restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);
    ASSERT_EQ(restored.attachLifecycle(config), diag::Result::Ok);

    EXPECT_EQ(restored.loadPersistent(), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = restored.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().lastResetReason, diag::ResetReason::Watchdog);
    EXPECT_EQ(snapshot.value().resetCount, 1U);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 1U);
    EXPECT_FALSE(snapshot.value().persistRequested);
    EXPECT_EQ(restored.dirtyFlags(), diag::DirtyFlags{0U});
}

TEST_F(StorageFixture, SaveDirtyDtcPreservesCleanLifecycleSection)
{
    std::array<diag::DtcRecord, 2U> records{};
    diag::ContextStorage            sourceStorage{};
    diag::Context               source{sourceStorage, diag::Config{records.data(), records.size()}};
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.attachLifecycle(config), diag::Result::Ok);
    ASSERT_EQ(source.observeReset(diag::ResetReason::Watchdog), diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);
    ASSERT_EQ(source.dirtyFlags(), diag::DirtyFlags{0U});

    ASSERT_EQ(source.registerDtc(diag::DtcId{0x313233U}, diag::DtcSeverity::Warning),
              diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    const diag::ResultValue<diag::CapsuleDescriptor> descriptor =
        diag::decodeCapsule(m_storage.persisted.data(), m_storage.persistedLength);
    ASSERT_TRUE(descriptor.hasValue());
    EXPECT_EQ(descriptor.value().sectionCount, 2U);

    const diag::ResultValue<diag::CapsuleSection> lifecycleSection = diag::findCapsuleSectionByType(
        descriptor.value(), static_cast<std::uint16_t>(diag::CapsuleSectionType::Lifecycle));
    ASSERT_TRUE(lifecycleSection.hasValue());

    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    std::array<diag::DtcRecord, 2U>         restoredRecords{};
    diag::ContextStorage                    restoredStorage{};
    diag::Config                            restoredConfig{};
    restoredConfig.dtcRecords = restoredRecords.data();
    restoredConfig.dtcCapacity = restoredRecords.size();
    diag::Context restored{restoredStorage, restoredConfig};
    diag::Storage restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);
    ASSERT_EQ(restored.attachLifecycle(config), diag::Result::Ok);
    ASSERT_EQ(restored.loadPersistent(), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = restored.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().lastResetReason, diag::ResetReason::Watchdog);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 1U);
    EXPECT_EQ(restored.dtcCount(), 1U);
}

TEST_F(StorageFixture, LoadRejectsLifecyclePolicyMismatch)
{
    diag::ContextStorage        sourceStorage{};
    diag::Context               source{sourceStorage};
    const diag::LifecycleConfig savedConfig{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};
    const diag::LifecycleConfig restoredConfig{diag::ResetCounterPolicy::EveryN, 4U, 0U};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.attachLifecycle(savedConfig), diag::Result::Ok);
    ASSERT_EQ(source.observeReset(diag::ResetReason::Watchdog), diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    diag::ContextStorage                    restoredStorage{};
    diag::Context                           restored{restoredStorage};
    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    diag::Storage                           restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);
    ASSERT_EQ(restored.attachLifecycle(restoredConfig), diag::Result::Ok);

    EXPECT_EQ(restored.loadPersistent(), diag::Result::CorruptData);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = restored.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().resetCounterPolicy, diag::ResetCounterPolicy::EveryN);
    EXPECT_EQ(snapshot.value().resetCount, 0U);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 0U);
}

TEST_F(StorageFixture, LoadSkipsDtcSectionWhenDtcStorageIsNotAttached)
{
    std::array<diag::DtcRecord, 2U> sourceRecords{};
    diag::ContextStorage            sourceStorage{};
    diag::Context source{sourceStorage, diag::Config{sourceRecords.data(), sourceRecords.size()}};
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.attachLifecycle(config), diag::Result::Ok);
    ASSERT_EQ(source.registerDtc(diag::DtcId{0x111213U}, diag::DtcSeverity::Warning),
              diag::Result::Ok);
    ASSERT_EQ(source.setDtcActive(diag::DtcId{0x111213U}, true), diag::Result::Ok);
    ASSERT_EQ(source.observeReset(diag::ResetReason::Fault), diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    diag::ContextStorage                    restoredStorage{};
    diag::Context                           restored{restoredStorage};
    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    diag::Storage                           restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);
    ASSERT_EQ(restored.attachLifecycle(config), diag::Result::Ok);

    EXPECT_EQ(restored.loadPersistent(), diag::Result::Ok);

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = restored.lifecycle();
    ASSERT_TRUE(snapshot.hasValue());
    EXPECT_EQ(snapshot.value().lastResetReason, diag::ResetReason::Fault);
    EXPECT_EQ(snapshot.value().abnormalResetCount, 1U);
    EXPECT_EQ(restored.dtcCount(), 0U);
}

TEST_F(StorageFixture, LoadSkipsLifecycleSectionWhenLifecycleIsNotAttached)
{
    std::array<diag::DtcRecord, 2U> sourceRecords{};
    diag::ContextStorage            sourceStorage{};
    diag::Context source{sourceStorage, diag::Config{sourceRecords.data(), sourceRecords.size()}};
    const diag::LifecycleConfig config{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};

    ASSERT_EQ(source.attachStorage(makeStorage()), diag::Result::Ok);
    ASSERT_EQ(source.attachLifecycle(config), diag::Result::Ok);
    ASSERT_EQ(source.registerDtc(diag::DtcId{0x212223U}, diag::DtcSeverity::Critical),
              diag::Result::Ok);
    ASSERT_EQ(source.setDtcActive(diag::DtcId{0x212223U}, true), diag::Result::Ok);
    ASSERT_EQ(source.observeReset(diag::ResetReason::Watchdog), diag::Result::Ok);
    ASSERT_EQ(source.savePersistent(), diag::Result::Ok);

    std::array<std::uint8_t, kStorageBytes> restoreCapsule{};
    std::array<diag::DtcRecord, 2U>         restoredRecords{};
    diag::ContextStorage                    restoredStorage{};
    diag::Config                            restoredConfig{};
    restoredConfig.dtcRecords = restoredRecords.data();
    restoredConfig.dtcCapacity = restoredRecords.size();
    diag::Context restored{restoredStorage, restoredConfig};
    diag::Storage restoreAdapter = makeStorage();
    restoreAdapter.capsuleBuffer = restoreCapsule.data();
    restoreAdapter.capsuleBufferSize = restoreCapsule.size();

    ASSERT_EQ(restored.attachStorage(restoreAdapter), diag::Result::Ok);

    EXPECT_EQ(restored.loadPersistent(), diag::Result::Ok);
    EXPECT_EQ(restored.dtcCount(), 1U);
    EXPECT_EQ(restored.lifecycle().result(), diag::Result::NotFound);

    const diag::ResultValue<diag::DtcRecord> record = restored.dtc(diag::DtcId{0x212223U});
    ASSERT_TRUE(record.hasValue());
    EXPECT_EQ(record.value().severity, diag::DtcSeverity::Critical);
}

TEST_F(StorageFixture, ClearPersistentInvokesAdapterClear)
{
    diag::ContextStorage contextStorage{};
    diag::Context        context{contextStorage};

    ASSERT_EQ(context.attachStorage(makeStorage()), diag::Result::Ok);

    EXPECT_EQ(context.clearPersistent(), diag::Result::Ok);
    EXPECT_EQ(m_storage.clearCalls, 1U);
}
