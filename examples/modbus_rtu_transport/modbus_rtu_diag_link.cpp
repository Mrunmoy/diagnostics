#include "modbus_rtu_diag_link.hpp"

#include <array>
#include <cstring>
#include <stdexcept>

namespace diag_modbus_rtu
{
namespace
{

constexpr std::size_t kHeaderSize = 4u;
constexpr std::size_t kCrcSize = 2u;
constexpr std::size_t kMaxAduSize = kHeaderSize + EXAMPLE_DIAG_MAX_FRAME_SIZE + kCrcSize;

std::uint16_t crc16(const std::uint8_t *data, std::size_t size)
{
    std::uint16_t crc = 0xFFFFu;

    for (std::size_t i = 0u; i < size; ++i)
    {
        crc ^= data[i];
        for (std::uint8_t bit = 0u; bit < 8u; ++bit)
        {
            if ((crc & 0x0001u) != 0u)
            {
                crc = static_cast<std::uint16_t>((crc >> 1u) ^ 0xA001u);
            }
            else
            {
                crc = static_cast<std::uint16_t>(crc >> 1u);
            }
        }
    }

    return crc;
}

bool readByteWithDeadline(diag_serial::SerialPort &port, std::uint8_t &outByte,
                          std::chrono::steady_clock::time_point deadline)
{
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (port.readByte(outByte, std::chrono::milliseconds{25}))
        {
            return true;
        }
    }

    return false;
}

bool receiveFrame(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                  struct example_diag_frame &outFrame, std::chrono::milliseconds timeout)
{
    const auto                            startedAt = std::chrono::steady_clock::now();
    const auto                            deadline = startedAt + timeout;
    std::array<std::uint8_t, kMaxAduSize> adu = {0};
    std::uint8_t                          byte = 0u;
    std::size_t                           payloadLength = 0u;
    std::size_t                           frameLength = 0u;

    while (std::chrono::steady_clock::now() < deadline)
    {
        if (!readByteWithDeadline(port, byte, deadline))
        {
            return false;
        }
        if (byte != slaveAddress)
        {
            continue;
        }

        adu[0] = byte;
        if (!readByteWithDeadline(port, adu[1], deadline))
        {
            return false;
        }
        if (adu[1] != kDiagnosticFunction)
        {
            continue;
        }
        if (!readByteWithDeadline(port, adu[2], deadline) ||
            !readByteWithDeadline(port, adu[3], deadline))
        {
            return false;
        }

        payloadLength = adu[2] | (static_cast<std::size_t>(adu[3]) << 8u);
        if (payloadLength > EXAMPLE_DIAG_MAX_FRAME_SIZE)
        {
            continue;
        }

        frameLength = kHeaderSize + payloadLength + kCrcSize;
        for (std::size_t i = kHeaderSize; i < frameLength; ++i)
        {
            if (!readByteWithDeadline(port, adu[i], deadline))
            {
                return false;
            }
        }

        const std::uint16_t expected = crc16(adu.data(), frameLength - kCrcSize);
        const std::uint16_t observed = static_cast<std::uint16_t>(
            adu[frameLength - 2u] | (static_cast<std::uint16_t>(adu[frameLength - 1u]) << 8u));
        if (observed != expected)
        {
            continue;
        }

        std::memcpy(outFrame.bytes, &adu[kHeaderSize], payloadLength);
        outFrame.size = payloadLength;
        return true;
    }

    return false;
}

void sendFrame(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
               const struct example_diag_frame &frame)
{
    std::array<std::uint8_t, kMaxAduSize> adu = {0};
    const std::size_t                     frameLength = kHeaderSize + frame.size + kCrcSize;

    if (frame.size > EXAMPLE_DIAG_MAX_FRAME_SIZE)
    {
        throw std::runtime_error("Modbus RTU diagnostic payload is too large");
    }

    adu[0] = slaveAddress;
    adu[1] = kDiagnosticFunction;
    adu[2] = static_cast<std::uint8_t>(frame.size & 0xFFu);
    adu[3] = static_cast<std::uint8_t>((frame.size >> 8u) & 0xFFu);
    std::memcpy(&adu[kHeaderSize], frame.bytes, frame.size);

    const std::uint16_t crc = crc16(adu.data(), frameLength - kCrcSize);
    adu[frameLength - 2u] = static_cast<std::uint8_t>(crc & 0xFFu);
    adu[frameLength - 1u] = static_cast<std::uint8_t>((crc >> 8u) & 0xFFu);

    port.writeAll(adu.data(), frameLength);
}

} // namespace

bool receiveRequest(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                    struct example_diag_frame &outFrame, std::chrono::milliseconds timeout)
{
    return receiveFrame(port, slaveAddress, outFrame, timeout);
}

bool receiveResponse(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                     struct example_diag_frame &outFrame, std::chrono::milliseconds timeout)
{
    return receiveFrame(port, slaveAddress, outFrame, timeout);
}

void sendRequest(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                 const struct example_diag_frame &frame)
{
    sendFrame(port, slaveAddress, frame);
}

void sendResponse(diag_serial::SerialPort &port, std::uint8_t slaveAddress,
                  const struct example_diag_frame &frame)
{
    sendFrame(port, slaveAddress, frame);
}

} // namespace diag_modbus_rtu
