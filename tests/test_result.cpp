#include "diag/result.hpp"

#include <gtest/gtest.h>

TEST(DiagResult, OkIsZeroForCStyleBoundaryCompatibility)
{
    EXPECT_EQ(static_cast<unsigned>(diag::Result::Ok), 0U);
    EXPECT_TRUE(diag::isOk(diag::Result::Ok));
    EXPECT_FALSE(diag::isOk(diag::Result::Capacity));
}

TEST(DiagResultValue, CarriesValueOnlyForOkResult)
{
    const diag::ResultValue<unsigned> value{42U};
    const diag::ResultValue<unsigned> error{diag::Result::NotFound};

    EXPECT_TRUE(value.hasValue());
    EXPECT_EQ(value.result(), diag::Result::Ok);
    EXPECT_EQ(value.value(), 42U);

    EXPECT_FALSE(error.hasValue());
    EXPECT_EQ(error.result(), diag::Result::NotFound);
}
