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

TEST(DiagContextStorage, HasStableBoundedSizeAndAlignment)
{
    EXPECT_EQ(diag::ContextStorage::kSize, 128U);
    EXPECT_GE(diag::ContextStorage::kAlignment, alignof(std::max_align_t));
    EXPECT_EQ(sizeof(diag::ContextStorage), diag::ContextStorage::kSize);
}
