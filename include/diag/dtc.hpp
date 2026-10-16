#pragma once

#include "diag/types.hpp"

#include <cstddef>
#include <cstdint>

namespace diag
{

using DtcStatusFlags = std::uint8_t;

constexpr std::uint8_t  kDtcDefaultConfirmationThreshold = 1U;
constexpr std::uint16_t kDtcDefaultAgingThreshold = 40U;
constexpr std::uint16_t kDtcNamespaceVersion = 1U;

enum class DtcStatus : DtcStatusFlags
{
    None = 0U,
    TestFailed = 1U << 0U,
    TestFailedThisCycle = 1U << 1U,
    Pending = 1U << 2U,
    Confirmed = 1U << 3U,
    TestNotCompletedSinceClear = 1U << 4U,
    TestFailedSinceClear = 1U << 5U,
    TestNotCompletedThisCycle = 1U << 6U,
    WarningIndicatorRequested = 1U << 7U,
};

constexpr DtcStatusFlags operator|(const DtcStatus lhs, const DtcStatus rhs)
{
    return static_cast<DtcStatusFlags>(lhs) | static_cast<DtcStatusFlags>(rhs);
}

enum class DtcSeverity : std::uint8_t
{
    Info = 0U,
    Warning = 1U,
    Error = 2U,
    Critical = 3U,
};

struct DtcRecord
{
    static constexpr std::size_t kEncodedSize = 28U;

    DtcId          id{};
    LocalFaultId   localFaultId{};
    DtcSeverity    severity{DtcSeverity::Info};
    bool           active{false};
    bool           failedThisCycle{false};
    DtcStatusFlags status{0U};
    std::uint8_t   failedCycleCount{0U};
    std::uint8_t   reserved0{0U};
    std::uint16_t  agingCounter{0U};
    std::uint32_t  occurrenceCount{0U};
    std::uint32_t  activeCount{0U};
    std::uint32_t  clearCount{0U};
};

static_assert(sizeof(DtcRecord) == DtcRecord::kEncodedSize,
              "DtcRecord layout must match the 28-byte capsule encoding");

constexpr bool hasStatus(const DtcRecord &record, const DtcStatus status)
{
    return (record.status & static_cast<DtcStatusFlags>(status)) != 0U;
}

} // namespace diag
