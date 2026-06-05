#include <gtest/gtest.h>

extern "C"
{
#include "diag/features.h"
}

namespace
{

static bool is_boolean_feature(int value)
{
    return value == 0 || value == 1;
}

TEST(DiagFeatures, DefinesBooleanFeatureSwitches)
{
    EXPECT_TRUE(is_boolean_feature(DIAG_FEATURE_DTC));
    EXPECT_TRUE(is_boolean_feature(DIAG_FEATURE_LIFECYCLE));
    EXPECT_TRUE(is_boolean_feature(DIAG_FEATURE_IDENTITY));
    EXPECT_TRUE(is_boolean_feature(DIAG_FEATURE_STORAGE));
    EXPECT_TRUE(is_boolean_feature(DIAG_FEATURE_TRANSPORT));
    EXPECT_TRUE(is_boolean_feature(DIAG_FEATURE_CAPSULE));
}

} // namespace
