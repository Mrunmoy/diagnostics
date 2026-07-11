#include "diag/context.hpp"

#include <cstddef>

#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
#include <array>
#include <cstdint>
#endif

#include <gtest/gtest.h>
#include <type_traits>

#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
namespace
{

struct ContextTestStorage
{
    std::array<std::uint8_t, 64U> persisted{};
    std::size_t                   saveCalls{0U};
};

diag::Result contextTestLoad(void *, std::uint8_t *, const std::size_t,
                             std::size_t &bytesRead) noexcept
{
    bytesRead = 0U;
    return diag::Result::Ok;
}

diag::Result contextTestSave(void *const user, const std::uint8_t *const,
                             const std::size_t) noexcept
{
    ContextTestStorage &storage = *static_cast<ContextTestStorage *>(user);
    ++storage.saveCalls;
    return diag::Result::Ok;
}

diag::Result contextTestClear(void *) noexcept
{
    return diag::Result::Ok;
}

diag::Storage makeContextTestStorage(ContextTestStorage &storage)
{
    return diag::Storage{
        diag::StorageOps{contextTestLoad, contextTestSave, contextTestClear},
        &storage,
        diag::StorageCapabilities{0xFFU, 8U},
        storage.persisted.data(),
        storage.persisted.size(),
    };
}

} // namespace
#endif

TEST(DiagContext, InitializesInCallerOwnedStorage)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};

    EXPECT_TRUE(context.isInitialized());
    EXPECT_EQ(context.dirtyFlags(), diag::DirtyFlags{0U});
}

TEST(DiagContext, IsNonCopyableAndNonMovable)
{
    EXPECT_FALSE(std::is_copy_constructible<diag::Context>::value);
    EXPECT_FALSE(std::is_copy_assignable<diag::Context>::value);
    EXPECT_FALSE(std::is_move_constructible<diag::Context>::value);
    EXPECT_FALSE(std::is_move_assignable<diag::Context>::value);
}

TEST(DiagContext, TracksDirtyFlagsExplicitly)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};

    EXPECT_EQ(context.markDirty(diag::DirtyFlag::Dtc), diag::Result::Ok);
    EXPECT_EQ(context.markDirty(diag::DirtyFlag::Lifecycle), diag::Result::Ok);
    EXPECT_EQ(context.dirtyFlags(), diag::DirtyFlag::Dtc | diag::DirtyFlag::Lifecycle);

    EXPECT_EQ(context.clearDirty(diag::DirtyFlag::Dtc), diag::Result::Ok);
    EXPECT_EQ(context.dirtyFlags(), static_cast<diag::DirtyFlags>(diag::DirtyFlag::Lifecycle));
}

#if !DIAG_FEATURE_IDENTITY
TEST(DiagContextIdentity, DisabledFeatureReportsNotSupported)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    const diag::Identity identity{};

    EXPECT_EQ(context.attachIdentity(identity), diag::Result::NotSupported);
    EXPECT_EQ(context.identity().result(), diag::Result::NotSupported);
}
#endif

#if DIAG_FEATURE_IDENTITY
TEST(DiagContextIdentity, ReportsMissingIdentityBeforeAttach)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};

    const diag::ResultValue<diag::Identity> identity = context.identity();

    EXPECT_FALSE(identity.hasValue());
    EXPECT_EQ(identity.result(), diag::Result::NotFound);
}

TEST(DiagContextIdentity, AttachesAndReturnsCompactIdentity)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    const diag::Identity configured{
        diag::EcosystemId{0x1001U},
        diag::ProductId{0x2002U},
        diag::DeviceType{0x3003U},
        diag::DeviceInstance{0x04U},
        diag::FirmwareStage{0x05U},
        diag::FirmwareComponent{0x06U},
        0U,
    };

    EXPECT_EQ(context.attachIdentity(configured), diag::Result::Ok);

    const diag::ResultValue<diag::Identity> actual = context.identity();
    ASSERT_TRUE(actual.hasValue());
    EXPECT_EQ(actual.value(), configured);
}
#endif

#if !DIAG_FEATURE_DTC
TEST(DiagContextDtc, DisabledFeatureReportsNotSupported)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    diag::DtcRecord      records[1]{};
    std::size_t          count = 7U;

    EXPECT_EQ(context.registerDtc(diag::DtcId{1U}, diag::DtcSeverity::Warning),
              diag::Result::NotSupported);
    EXPECT_EQ(context.dtc(diag::DtcId{1U}).result(), diag::Result::NotSupported);
    EXPECT_EQ(context.listDtcs(records, 1U, count), diag::Result::NotSupported);
    EXPECT_EQ(count, 0U);
    EXPECT_EQ(context.setDtcActive(diag::DtcId{1U}, true), diag::Result::NotSupported);
    EXPECT_EQ(context.clearDtc(diag::DtcId{1U}), diag::Result::NotSupported);
    EXPECT_EQ(context.dtcCount(), 0U);
}
#endif

#if !DIAG_FEATURE_LIFECYCLE
TEST(DiagContextLifecycle, DisabledFeatureReportsNotSupported)
{
    diag::ContextStorage        storage{};
    diag::Context               context{storage};
    const diag::LifecycleConfig config{};

    EXPECT_EQ(context.attachLifecycle(config), diag::Result::NotSupported);
    EXPECT_EQ(context.lifecycle().result(), diag::Result::NotSupported);
    EXPECT_EQ(context.observeReset(diag::ResetReason::Watchdog), diag::Result::NotSupported);
    EXPECT_EQ(context.clearLifecycleDirty(
                  static_cast<diag::LifecycleDirtyFlags>(diag::LifecycleDirtyFlag::ResetCounter)),
              diag::Result::NotSupported);
}
#endif

TEST(DiagContextStorage, HasStableBoundedSizeAndAlignment)
{
#if DIAG_FEATURE_DTC && DIAG_FEATURE_LIFECYCLE && DIAG_FEATURE_IDENTITY && DIAG_FEATURE_STORAGE && \
    DIAG_FEATURE_CAPSULE
    EXPECT_EQ(diag::ContextStorage::kSize, 176U);
#else
    EXPECT_LT(diag::ContextStorage::kSize, 176U);
#endif
    EXPECT_GE(diag::ContextStorage::kAlignment, alignof(std::max_align_t));
    EXPECT_EQ(sizeof(diag::ContextStorage), diag::ContextStorage::kSize);
}

TEST(DiagContextStorage, RejectsSecondLiveContextOnSameStorage)
{
    diag::ContextStorage storage{};
    diag::Context        first{storage};
    diag::Context        second{storage};

    EXPECT_TRUE(first.isInitialized());
    EXPECT_FALSE(second.isInitialized());
}

TEST(DiagContextStorage, ReusesStorageAfterFirstContextIsDestroyed)
{
    diag::ContextStorage storage{};

    {
        diag::Context first{storage};
        ASSERT_TRUE(first.isInitialized());
    }

    diag::Context second{storage};
    EXPECT_TRUE(second.isInitialized());
}

#if !(DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE)
TEST(DiagContextPersistence, DisabledFeatureReportsNotSupported)
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    diag::Storage        adapter{};

    EXPECT_EQ(context.attachStorage(adapter), diag::Result::NotSupported);
    EXPECT_EQ(context.savePersistent(), diag::Result::NotSupported);
    EXPECT_EQ(context.loadPersistent(), diag::Result::NotSupported);
    EXPECT_EQ(context.clearPersistent(), diag::Result::NotSupported);
}
#endif

#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE && !DIAG_FEATURE_DTC
TEST(DiagContextPersistence, RejectsDtcDirtyFlagWhenDtcFeatureIsDisabled)
{
    ContextTestStorage   storageAdapter{};
    diag::ContextStorage contextStorage{};
    diag::Context        context{contextStorage};

    ASSERT_EQ(context.attachStorage(makeContextTestStorage(storageAdapter)), diag::Result::Ok);
    ASSERT_EQ(context.markDirty(diag::DirtyFlag::Dtc), diag::Result::Ok);

    EXPECT_EQ(context.savePersistent(), diag::Result::NotSupported);
    EXPECT_EQ(storageAdapter.saveCalls, 0U);
}
#endif

#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE && !DIAG_FEATURE_LIFECYCLE
TEST(DiagContextPersistence, RejectsLifecycleDirtyFlagWhenLifecycleFeatureIsDisabled)
{
    ContextTestStorage   storageAdapter{};
    diag::ContextStorage contextStorage{};
    diag::Context        context{contextStorage};

    ASSERT_EQ(context.attachStorage(makeContextTestStorage(storageAdapter)), diag::Result::Ok);
    ASSERT_EQ(context.markDirty(diag::DirtyFlag::Lifecycle), diag::Result::Ok);

    EXPECT_EQ(context.savePersistent(), diag::Result::NotSupported);
    EXPECT_EQ(storageAdapter.saveCalls, 0U);
}
#endif

#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
TEST(DiagContextPersistence, RejectsUnknownDirtyFlagBeforeEncodingEmptyCapsule)
{
    constexpr diag::DirtyFlags kUnknownDirtyFlag = 1U << 8U;

    ContextTestStorage   storageAdapter{};
    diag::ContextStorage contextStorage{};
    diag::Context        context{contextStorage};

    ASSERT_EQ(context.attachStorage(makeContextTestStorage(storageAdapter)), diag::Result::Ok);
    ASSERT_EQ(context.markDirty(static_cast<diag::DirtyFlag>(kUnknownDirtyFlag)), diag::Result::Ok);

    EXPECT_EQ(context.savePersistent(), diag::Result::NotSupported);
    EXPECT_EQ(storageAdapter.saveCalls, 0U);
}
#endif

#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE && DIAG_FEATURE_DTC
TEST(DiagContextPersistence, RejectsUnknownDirtyFlagCombinedWithKnownDirtyFlag)
{
    constexpr diag::DirtyFlags kUnknownDirtyFlag = 1U << 8U;

    ContextTestStorage   storageAdapter{};
    diag::DtcRecord      records[1]{};
    diag::ContextStorage contextStorage{};
    diag::Config         config{};
    config.dtcRecords = records;
    config.dtcCapacity = 1U;
    diag::Context context{contextStorage, config};

    ASSERT_EQ(context.attachStorage(makeContextTestStorage(storageAdapter)), diag::Result::Ok);
    ASSERT_EQ(context.registerDtc(diag::DtcId{0x010203U}, diag::DtcSeverity::Warning),
              diag::Result::Ok);
    ASSERT_EQ(context.markDirty(static_cast<diag::DirtyFlag>(kUnknownDirtyFlag)), diag::Result::Ok);

    EXPECT_EQ(context.savePersistent(), diag::Result::NotSupported);
    EXPECT_EQ(storageAdapter.saveCalls, 0U);
    EXPECT_EQ(context.dirtyFlags(),
              (static_cast<diag::DirtyFlags>(diag::DirtyFlag::Dtc) | kUnknownDirtyFlag));
}
#endif
