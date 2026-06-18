#include "diag/context.hpp"

#include <gtest/gtest.h>
#include <type_traits>

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

TEST(DiagContextStorage, HasStableBoundedSizeAndAlignment)
{
    EXPECT_EQ(diag::ContextStorage::kSize, 128U);
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
