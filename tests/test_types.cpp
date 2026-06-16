#include "diag/types.hpp"

#include <gtest/gtest.h>
#include <type_traits>

TEST(DiagStrongTypes, DoNotImplicitlyCollapseToRawIntegers)
{
    EXPECT_FALSE((std::is_convertible<diag::DtcId, std::uint32_t>::value));
    EXPECT_FALSE((std::is_convertible<std::uint32_t, diag::DtcId>::value));
    EXPECT_FALSE((std::is_convertible<diag::LocalFaultId, std::uint32_t>::value));
}

TEST(DiagStrongTypes, CompareSameSemanticType)
{
    EXPECT_EQ(diag::DtcId{0x040101U}, diag::DtcId{0x040101U});
    EXPECT_NE(diag::DtcId{0x040101U}, diag::DtcId{0x040102U});

    EXPECT_EQ(diag::LocalFaultId{7U}, diag::LocalFaultId{7U});
    EXPECT_NE(diag::LocalFaultId{7U}, diag::LocalFaultId{8U});
}
