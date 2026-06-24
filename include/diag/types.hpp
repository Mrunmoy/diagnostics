#pragma once

#include <cstdint>

namespace diag
{

struct DtcId
{
    std::uint32_t value{0U};
};

struct LocalFaultId
{
    std::uint32_t value{0U};
};

struct ProductId
{
    std::uint16_t value{0U};
};

struct DeviceInstance
{
    std::uint16_t value{0U};
};

constexpr bool operator==(const ProductId lhs, const ProductId rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const ProductId lhs, const ProductId rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const DeviceInstance lhs, const DeviceInstance rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const DeviceInstance lhs, const DeviceInstance rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const DtcId lhs, const DtcId rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const DtcId lhs, const DtcId rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const LocalFaultId lhs, const LocalFaultId rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const LocalFaultId lhs, const LocalFaultId rhs)
{
    return !(lhs == rhs);
}

} // namespace diag
