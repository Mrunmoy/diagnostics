#pragma once

#include "diag/types.hpp"

#include <cstddef>
#include <cstdint>

namespace diag
{

struct Identity
{
    static constexpr std::uint8_t kSchemaVersion = 1U;
    static constexpr std::size_t  kEncodedSize = 10U;

    EcosystemId       ecosystem{};
    ProductId         product{};
    DeviceType        deviceType{};
    DeviceInstance    instance{};
    FirmwareStage     firmwareStage{};
    FirmwareComponent firmwareComponent{};
    std::uint8_t      reserved{0U};
};

constexpr bool operator==(const Identity lhs, const Identity rhs)
{
    return (lhs.ecosystem == rhs.ecosystem) && (lhs.product == rhs.product) &&
           (lhs.deviceType == rhs.deviceType) && (lhs.instance == rhs.instance) &&
           (lhs.firmwareStage == rhs.firmwareStage) &&
           (lhs.firmwareComponent == rhs.firmwareComponent) && (lhs.reserved == rhs.reserved);
}

constexpr bool operator!=(const Identity lhs, const Identity rhs)
{
    return !(lhs == rhs);
}

} // namespace diag
