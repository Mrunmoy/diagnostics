#include "diag/identity.hpp"

#include <gtest/gtest.h>
TEST(DiagIdentity, HasCompactNumericStorageShape)
{
    EXPECT_EQ(diag::Identity::kSchemaVersion, 1U);
    EXPECT_EQ(diag::Identity::kEncodedSize, 10U);
    EXPECT_EQ(sizeof(diag::Identity), diag::Identity::kEncodedSize);
}

TEST(DiagIdentity, ComparesEveryField)
{
    const diag::Identity left{
        diag::EcosystemId{1U},
        diag::ProductId{2U},
        diag::DeviceType{3U},
        diag::DeviceInstance{4U},
        diag::FirmwareStage{5U},
        diag::FirmwareComponent{6U},
        0U,
    };
    diag::Identity right = left;

    EXPECT_EQ(left, right);

    right.firmwareComponent = diag::FirmwareComponent{7U};
    EXPECT_NE(left, right);
}
