#include "diag/result.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
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

class ThrowingCopyConstructible
{
  public:
    explicit ThrowingCopyConstructible(const unsigned value) : m_value{value}
    {
        ++s_constructed;
    }

    ThrowingCopyConstructible(const ThrowingCopyConstructible &)
    {
        throw std::runtime_error{"copy"};
    }

    ThrowingCopyConstructible(ThrowingCopyConstructible &&other) noexcept : m_value{other.m_value}
    {
        ++s_constructed;
    }

    ~ThrowingCopyConstructible()
    {
        --s_constructed;
    }

    [[nodiscard]] static int constructed()
    {
        return s_constructed;
    }

  private:
    unsigned   m_value;
    static int s_constructed;
};

int ThrowingCopyConstructible::s_constructed = 0;

class ThrowingMoveConstructible
{
  public:
    explicit ThrowingMoveConstructible(const unsigned value) : m_value{value}
    {
        ++s_constructed;
    }

    ThrowingMoveConstructible(const ThrowingMoveConstructible &other) : m_value{other.m_value}
    {
        ++s_constructed;
    }

    ThrowingMoveConstructible(ThrowingMoveConstructible &&)
    {
        throw std::runtime_error{"move"};
    }

    ~ThrowingMoveConstructible()
    {
        --s_constructed;
    }

    [[nodiscard]] static int constructed()
    {
        return s_constructed;
    }

  private:
    unsigned   m_value;
    static int s_constructed;
};

int ThrowingMoveConstructible::s_constructed = 0;

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
    const unsigned                   *checked_value = value.valueOrNull();

    EXPECT_TRUE(value.hasValue());
    EXPECT_EQ(value.result(), diag::Result::Ok);
    EXPECT_EQ(value.value(), 42U);
    ASSERT_NE(checked_value, nullptr);
    EXPECT_EQ(*checked_value, 42U);

    EXPECT_FALSE(error.hasValue());
    EXPECT_EQ(error.result(), diag::Result::NotFound);
    EXPECT_EQ(error.valueOrNull(), nullptr);
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

TEST(DiagResultValue, CopyAssignmentKeepsObjectInvalidWhenCopyConstructionThrows)
{
    EXPECT_EQ(ThrowingCopyConstructible::constructed(), 0);

    {
        diag::ResultValue<ThrowingCopyConstructible> source{ThrowingCopyConstructible{1U}};
        diag::ResultValue<ThrowingCopyConstructible> target{ThrowingCopyConstructible{2U}};

        EXPECT_THROW(target = source, std::runtime_error);
        EXPECT_FALSE(target.hasValue());
        EXPECT_EQ(target.result(), diag::Result::InvalidArgument);
        EXPECT_EQ(ThrowingCopyConstructible::constructed(), 1);
    }

    EXPECT_EQ(ThrowingCopyConstructible::constructed(), 0);
}

TEST(DiagResultValue, MoveAssignmentKeepsObjectInvalidWhenMoveConstructionThrows)
{
    EXPECT_EQ(ThrowingMoveConstructible::constructed(), 0);

    {
        const ThrowingMoveConstructible              source_value{1U};
        const ThrowingMoveConstructible              target_value{2U};
        diag::ResultValue<ThrowingMoveConstructible> source{source_value};
        diag::ResultValue<ThrowingMoveConstructible> target{target_value};

        EXPECT_THROW(target = std::move(source), std::runtime_error);
        EXPECT_FALSE(target.hasValue());
        EXPECT_EQ(target.result(), diag::Result::InvalidArgument);
        EXPECT_EQ(ThrowingMoveConstructible::constructed(), 3);
    }

    EXPECT_EQ(ThrowingMoveConstructible::constructed(), 0);
}
