#pragma once

#include <cstdint>

namespace diag::internal
{

inline void writeU16Le(std::uint8_t *const buffer, const std::uint16_t value) noexcept
{
    buffer[0] = static_cast<std::uint8_t>(value & 0xFFU);
    buffer[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

inline void writeU32Le(std::uint8_t *const buffer, const std::uint32_t value) noexcept
{
    buffer[0] = static_cast<std::uint8_t>(value & 0xFFU);
    buffer[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buffer[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    buffer[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

[[nodiscard]] inline std::uint16_t readU16Le(const std::uint8_t *const buffer) noexcept
{
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(buffer[0]) |
                                      (static_cast<std::uint16_t>(buffer[1]) << 8U));
}

[[nodiscard]] inline std::uint32_t readU32Le(const std::uint8_t *const buffer) noexcept
{
    return static_cast<std::uint32_t>(static_cast<std::uint32_t>(buffer[0]) |
                                      (static_cast<std::uint32_t>(buffer[1]) << 8U) |
                                      (static_cast<std::uint32_t>(buffer[2]) << 16U) |
                                      (static_cast<std::uint32_t>(buffer[3]) << 24U));
}

} // namespace diag::internal
