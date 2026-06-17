#pragma once

#include <cstdint>

namespace diag
{

enum class Result : std::uint8_t
{
    Ok = 0,
    InvalidArgument,
    NotInitialized,
    NotFound,
    AlreadyExists,
    Capacity,
    Storage,
    Transport,
    CorruptData,
    NotSupported,
};

[[nodiscard]] constexpr bool isOk(const Result result)
{
    return result == Result::Ok;
}

template <typename T> class ResultValue
{
  public:
    constexpr ResultValue(const Result result) : m_result{result}, m_value{} {}

    constexpr ResultValue(const T &value) : m_result{Result::Ok}, m_value{value} {}

    [[nodiscard]] constexpr Result result() const
    {
        return m_result;
    }

    [[nodiscard]] constexpr bool hasValue() const
    {
        return m_result == Result::Ok;
    }

    [[nodiscard]] constexpr const T &value() const
    {
        return m_value;
    }

  private:
    Result m_result;
    T      m_value;
};

} // namespace diag
