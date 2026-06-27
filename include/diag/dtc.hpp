#pragma once

#include "diag/types.hpp"

#include <cstddef>
#include <cstdint>

namespace diag
{

using DtcStatusFlags = std::uint8_t;

enum class DtcStatus : DtcStatusFlags
{
    None = 0U,
    TestFailed = 1U << 0U,
    Pending = 1U << 1U,
    Confirmed = 1U << 2U,
    TestFailedThisCycle = 1U << 3U,
};

constexpr DtcStatusFlags operator|(const DtcStatus lhs, const DtcStatus rhs)
{
    return static_cast<DtcStatusFlags>(lhs) | static_cast<DtcStatusFlags>(rhs);
}

enum class DtcSeverity : std::uint8_t
{
    Info = 0U,
    Warning = 1U,
    Critical = 2U,
};

struct DtcRecord
{
    static constexpr std::size_t kEncodedSize = 16U;

    DtcId          id{};
    std::uint32_t  occurrenceCount{0U};
    std::uint32_t  clearCount{0U};
    DtcStatusFlags status{0U};
    DtcSeverity    severity{DtcSeverity::Info};
    std::uint8_t   reserved0{0U};
    std::uint8_t   reserved1{0U};
};

static_assert(sizeof(DtcRecord) == DtcRecord::kEncodedSize,
              "DtcRecord layout must stay compact and explicit");

constexpr bool hasStatus(const DtcRecord &record, const DtcStatus status)
{
    return (record.status & static_cast<DtcStatusFlags>(status)) != 0U;
}

} // namespace diag
