#pragma once

#include <cstdint>

namespace diag
{

struct DtcId
{
    std::uint32_t value{0U};
};

struct EcosystemId
{
    std::uint16_t value{0U};
};

struct LocalFaultId
{
    std::uint32_t value{0U};
};

struct ProductId
{
    std::uint16_t value{0U};
};

struct DeviceType
{
    std::uint16_t value{0U};
};

struct DeviceInstance
{
    std::uint8_t value{0U};
};

struct FirmwareStage
{
    std::uint8_t value{0U};
};

struct FirmwareComponent
{
    std::uint8_t value{0U};
};

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

constexpr bool operator==(const EcosystemId lhs, const EcosystemId rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const EcosystemId lhs, const EcosystemId rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const ProductId lhs, const ProductId rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const ProductId lhs, const ProductId rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const DeviceType lhs, const DeviceType rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const DeviceType lhs, const DeviceType rhs)
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

constexpr bool operator==(const FirmwareStage lhs, const FirmwareStage rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const FirmwareStage lhs, const FirmwareStage rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const FirmwareComponent lhs, const FirmwareComponent rhs)
{
    return lhs.value == rhs.value;
}

constexpr bool operator!=(const FirmwareComponent lhs, const FirmwareComponent rhs)
{
    return !(lhs == rhs);
}

} // namespace diag
