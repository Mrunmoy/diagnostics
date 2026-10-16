#pragma once

#include "diag/diag.hpp"

#include <cstddef>
#include <cstdint>

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

void          writeU32Le(std::uint8_t *buffer, std::uint32_t value) noexcept;
std::uint32_t readU32Le(const std::uint8_t *buffer) noexcept;

[[nodiscard]] diag::Result handleRequest(const Device &device, const Frame &request,
                                         Frame &response) noexcept;

} // namespace example_diag
