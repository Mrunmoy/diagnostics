#pragma once

#include "diag/diag.hpp"

#include <cstddef>
#include <cstdint>
#include <iosfwd>

namespace example_diag
{

constexpr std::size_t kMaxFrameSize = 160U;
constexpr std::size_t kDtcWireSize = 10U;
constexpr std::size_t kMaxDtcSnapshotCount = 8U;

enum class Service : std::uint8_t
{
    ReadIdentity = 0x01U,
    ListDtcs = 0x02U,
    ClearDtc = 0x03U,
};

struct Frame
{
    std::uint8_t bytes[kMaxFrameSize]{};
    std::size_t  size{0U};
};

struct Device
{
    const char    *name{nullptr};
    diag::Context *context{nullptr};
    std::size_t (*persistedSize)(void *user){nullptr};
    void *user{nullptr};
};

struct IdentitySnapshot
{
    std::uint16_t ecosystemId{0U};
    std::uint16_t productId{0U};
    std::uint16_t deviceType{0U};
    std::uint8_t  deviceInstance{0U};
    std::uint8_t  firmwareStage{0U};
    std::uint8_t  firmwareComponent{0U};
};

struct DtcSnapshot
{
    diag::DtcId   id{};
    std::uint8_t  status{0U};
    std::uint8_t  severity{0U};
    std::uint32_t occurrenceCount{0U};
};

struct ToolSnapshot
{
    IdentitySnapshot identity{};
    DtcSnapshot      dtcs[kMaxDtcSnapshotCount]{};
    std::size_t      dtcCount{0U};
    std::size_t      persistedSize{0U};
};

void          writeU32Le(std::uint8_t *buffer, std::uint32_t value) noexcept;
std::uint32_t readU32Le(const std::uint8_t *buffer) noexcept;

[[nodiscard]] diag::Result handleRequest(const Device &device, const Frame &request,
                                         Frame &response) noexcept;

[[nodiscard]] diag::Result collectSnapshot(const Device &device,
                                           ToolSnapshot &outSnapshot) noexcept;

[[nodiscard]] diag::Result runCli(const Device &device, diag::DtcId clearDtcId, std::ostream &out,
                                  std::ostream &err);

} // namespace example_diag
