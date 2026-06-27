#pragma once

#include <cstdint>

namespace diag
{

enum class ResetReason : std::uint8_t
{
    Unknown = 0U,
    PowerOn,
    Software,
    Watchdog,
    Brownout,
    External,
    Update,
    Fault,
};

enum class ResetCounterPolicy : std::uint8_t
{
    Disabled = 0U,
    RamOnly,
    AbnormalOnly,
    EveryN,
    Platform,
};

enum class LifecycleDirtyFlag : std::uint32_t
{
    None = 0U,
    ResetCounter = 1U << 0U,
};

using LifecycleDirtyFlags = std::uint32_t;

constexpr LifecycleDirtyFlags operator|(const LifecycleDirtyFlag lhs, const LifecycleDirtyFlag rhs)
{
    return static_cast<LifecycleDirtyFlags>(lhs) | static_cast<LifecycleDirtyFlags>(rhs);
}

struct LifecycleConfig
{
    ResetCounterPolicy resetCounterPolicy{ResetCounterPolicy::Disabled};
    std::uint32_t      resetCountInterval{0U};
    std::uint32_t      platformResetCount{0U};
};

struct LifecycleSnapshot
{
    ResetReason         lastResetReason{ResetReason::Unknown};
    ResetCounterPolicy  resetCounterPolicy{ResetCounterPolicy::Disabled};
    std::uint32_t       resetCount{0U};
    std::uint32_t       abnormalResetCount{0U};
    LifecycleDirtyFlags dirtyFlags{0U};
    bool                persistRequested{false};
};

[[nodiscard]] constexpr bool isAbnormalReset(const ResetReason reason)
{
    return reason == ResetReason::Watchdog || reason == ResetReason::Brownout ||
           reason == ResetReason::Fault;
}

} // namespace diag
