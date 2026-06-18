#include "diag/types.hpp"

#include <gtest/gtest.h>
#include <type_traits>

TEST(DiagStrongTypes, DoNotImplicitlyCollapseToRawIntegers)
{
    EXPECT_FALSE((std::is_convertible<diag::DtcId, std::uint32_t>::value));
    EXPECT_FALSE((std::is_convertible<std::uint32_t, diag::DtcId>::value));
    EXPECT_FALSE((std::is_convertible<diag::LocalFaultId, std::uint32_t>::value));
    EXPECT_FALSE((std::is_convertible<std::uint32_t, diag::LocalFaultId>::value));
    EXPECT_FALSE((std::is_convertible<diag::ProductId, std::uint16_t>::value));
    EXPECT_FALSE((std::is_convertible<std::uint16_t, diag::ProductId>::value));
    EXPECT_FALSE((std::is_convertible<diag::DeviceInstance, std::uint8_t>::value));
    EXPECT_FALSE((std::is_convertible<std::uint8_t, diag::DeviceInstance>::value));
}

TEST(DiagStrongTypes, CompareSameSemanticType)
{
    EXPECT_EQ(diag::DtcId{0x040101U}, diag::DtcId{0x040101U});
    EXPECT_NE(diag::DtcId{0x040101U}, diag::DtcId{0x040102U});

    EXPECT_EQ(diag::LocalFaultId{7U}, diag::LocalFaultId{7U});
    EXPECT_NE(diag::LocalFaultId{7U}, diag::LocalFaultId{8U});

    EXPECT_EQ(diag::EcosystemId{1U}, diag::EcosystemId{1U});
    EXPECT_NE(diag::ProductId{2U}, diag::ProductId{3U});
    EXPECT_EQ(diag::DeviceType{4U}, diag::DeviceType{4U});
    EXPECT_NE(diag::DeviceInstance{5U}, diag::DeviceInstance{6U});
    EXPECT_EQ(diag::FirmwareStage{7U}, diag::FirmwareStage{7U});
    EXPECT_NE(diag::FirmwareComponent{8U}, diag::FirmwareComponent{9U});
}
