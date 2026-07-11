#include "diag/features.hpp"

#include <gtest/gtest.h>

namespace
{

bool isBooleanFeature(const int value)
{
    return value == 0 || value == 1;
}

} // namespace

TEST(DiagFeatures, DefinesBooleanFeatureSwitches)
{
    EXPECT_TRUE(isBooleanFeature(DIAG_FEATURE_DTC));
    EXPECT_TRUE(isBooleanFeature(DIAG_FEATURE_LIFECYCLE));
    EXPECT_TRUE(isBooleanFeature(DIAG_FEATURE_IDENTITY));
    EXPECT_TRUE(isBooleanFeature(DIAG_FEATURE_STORAGE));
    EXPECT_TRUE(isBooleanFeature(DIAG_FEATURE_CAPSULE));
}
