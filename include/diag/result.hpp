#pragma once

#include <cstdint>
#include <new>
#include <type_traits>
#include <utility

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
    explicit ResultValue(const Result result) noexcept
        : m_result{result == Result::Ok ? Result::InvalidArgument : result}
    {
    }

    explicit ResultValue(const T &value) : m_result{Result::Ok}
    {
        construct(value);
    }

    explicit ResultValue(T &&value) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_result{Result::Ok}
    {
        construct(std::move(value));
    }

    ResultValue(const ResultValue &other) : m_result{other.m_result}
    {
        if (other.hasValue())
        {
            construct(other.value());
        }
    }

    ResultValue(ResultValue &&other) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_result{other.m_result}
    {
        if (other.hasValue())
        {
            construct(std::move(other.valueRef()));
        }
    }

    ~ResultValue() noexcept
    {
        destroy();
    }

    ResultValue &operator=(const ResultValue &other)
    {
        if (this != &other)
        {
            destroy();
            m_result = Result::InvalidArgument;
            if (other.hasValue())
            {
                construct(other.value());
                m_result = Result::Ok;
            }
            else
            {
                m_result = other.m_result;
            }
        }

        return *this;
    }

    ResultValue &
    operator=(ResultValue &&other) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        if (this != &other)
        {
            destroy();
            m_result = Result::InvalidArgument;
            if (other.hasValue())
            {
                construct(std::move(other.valueRef()));
                m_result = Result::Ok;
            }
            else
            {
                m_result = other.m_result;
            }
        }

        return *this;
    }

    [[nodiscard]] Result result() const noexcept
    {
        return m_result;
    }

    [[nodiscard]] bool hasValue() const noexcept
    {
        return m_result == Result::Ok;
    }

    [[nodiscard]] const T &value() const noexcept
    {
        return valueRef();
    }

    [[nodiscard]] const T *valueOrNull() const noexcept
    {
        return hasValue() ? &valueRef() : nullptr;
    }

  private:
    template <typename U>
    void construct(U &&value) noexcept(std::is_nothrow_constructible<T, U &&>::value)
    {
        new (static_cast<void *>(m_storage)) T{std::forward<U>(value)};
    }

    void destroy() noexcept
    {
        if (hasValue())
        {
            valueRef().~T();
        }
    }

    [[nodiscard]] T &valueRef() noexcept
    {
        return *std::launder(reinterpret_cast<T *>(m_storage));
    }

    [[nodiscard]] const T &valueRef() const noexcept
    {
        return *std::launder(reinterpret_cast<const T *>(m_storage));
    }

    Result m_result;
    alignas(T) unsigned char m_storage[sizeof(T)];
};

} // namespace diag
