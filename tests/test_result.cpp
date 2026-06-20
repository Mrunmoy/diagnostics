#include "diag/result.hpp"

#include <gtest/gtest.h>
#include <type_traits>

namespace
{

class NonDefaultConstructible
{
  public:
    explicit NonDefaultConstructible(const unsigned value) : m_value{value}
    {
        ++s_constructed;
    }

    NonDefaultConstructible(const NonDefaultConstructible &other) : m_value{other.m_value}
    {
        ++s_constructed;
    }

    ~NonDefaultConstructible()
    {
        --s_constructed;
    }

    [[nodiscard]] unsigned value() const
    {
        return m_value;
    }

    [[nodiscard]] static int constructed()
    {
        return s_constructed;
    }

  private:
    unsigned   m_value;
    static int s_constructed;
};

int NonDefaultConstructible::s_constructed = 0;

} // namespace

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

TEST(DiagResultValue, RejectsOkWithoutExplicitValue)
{
    const diag::ResultValue<unsigned> invalid{diag::Result::Ok};

    EXPECT_FALSE(invalid.hasValue());
    EXPECT_EQ(invalid.result(), diag::Result::InvalidArgument);
}

TEST(DiagResultValue, ErrorDoesNotConstructValueStorage)
{
    static_assert(!std::is_default_constructible<NonDefaultConstructible>::value,
                  "test fixture must prove ResultValue error path avoids default construction");

    EXPECT_EQ(NonDefaultConstructible::constructed(), 0);

    {
        const diag::ResultValue<NonDefaultConstructible> error{diag::Result::NotFound};

        EXPECT_FALSE(error.hasValue());
        EXPECT_EQ(error.result(), diag::Result::NotFound);
        EXPECT_EQ(NonDefaultConstructible::constructed(), 0);
    }

    EXPECT_EQ(NonDefaultConstructible::constructed(), 0);
}
