#pragma once

#include "diag/result.hpp"

#include <cstddef>
#include <cstdint>

namespace diag
{

using TransportSend = Result (*)(void *user, const std::uint8_t *buffer, std::size_t size) noexcept;
using TransportReceive = Result (*)(void *user, std::uint8_t *buffer, std::size_t bufferSize,
                                    std::size_t &bytesRead) noexcept;

struct TransportOps
{
    TransportSend    send{nullptr};
    TransportReceive receive{nullptr};
};

struct Transport
{
    TransportOps ops{};
    void        *user{nullptr};
};

[[nodiscard]] Result validateTransport(const Transport &transport) noexcept;
[[nodiscard]] Result transportSend(const Transport &transport, const std::uint8_t *buffer,
                                   std::size_t size) noexcept;
[[nodiscard]] Result transportReceive(const Transport &transport, std::uint8_t *buffer,
                                      std::size_t bufferSize, std::size_t &bytesRead) noexcept;

} // namespace diag
