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
    EXPECT_FALSE((std::is_convertible<diag::DeviceInstance, std::uint16_t>::value));
    EXPECT_FALSE((std::is_convertible<std::uint16_t, diag::DeviceInstance>::value));
}

TEST(DiagStrongTypes, CompareSameSemanticType)
{
    EXPECT_EQ(diag::DtcId{0x040101U}, diag::DtcId{0x040101U});
    EXPECT_NE(diag::DtcId{0x040101U}, diag::DtcId{0x040102U});

    EXPECT_EQ(diag::LocalFaultId{7U}, diag::LocalFaultId{7U});
    EXPECT_NE(diag::LocalFaultId{7U}, diag::LocalFaultId{8U});
}
