#include "diag/transport.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

namespace
{

struct MemoryTransport
{
    std::array<std::uint8_t, 32U> lastSent{};
    std::size_t                   lastSentSize{0U};
    std::size_t                   sendCalls{0U};
    std::size_t                   receiveCalls{0U};
};

diag::Result memorySend(void *const user, const std::uint8_t *const buffer,
                        const std::size_t size) noexcept
{
    MemoryTransport &transport = *static_cast<MemoryTransport *>(user);
    ++transport.sendCalls;
    transport.lastSentSize = size;
    for (std::size_t index = 0U; index < size && index < transport.lastSent.size(); ++index)
    {
        transport.lastSent[index] = buffer[index];
    }
    return diag::Result::Ok;
}

diag::Result memoryReceive(void *const user, std::uint8_t *const buffer,
                           const std::size_t bufferSize, std::size_t &bytesRead) noexcept
{
    MemoryTransport &transport = *static_cast<MemoryTransport *>(user);
    ++transport.receiveCalls;
    bytesRead = (bufferSize < 1U) ? 0U : 1U;
    if (bytesRead != 0U)
    {
        buffer[0] = 0xABU;
    }
    return diag::Result::Ok;
}

diag::Result oversizedReceive(void *, std::uint8_t *, const std::size_t bufferSize,
                              std::size_t &bytesRead) noexcept
{
    bytesRead = bufferSize + 1U;
    return diag::Result::Ok;
}

} // namespace

TEST(DiagTransport, ValidatesCallbacks)
{
    MemoryTransport transportState{};
    diag::Transport transport{diag::TransportOps{memorySend, memoryReceive}, &transportState};
    EXPECT_EQ(diag::validateTransport(transport), diag::Result::Ok);

    transport.ops.send = nullptr;
    EXPECT_EQ(diag::validateTransport(transport), diag::Result::InvalidArgument);

    transport.ops.send = memorySend;
    transport.ops.receive = nullptr;
    EXPECT_EQ(diag::validateTransport(transport), diag::Result::InvalidArgument);
}

TEST(DiagTransport, SendAndReceiveDelegateToAdapter)
{
    MemoryTransport       transportState{};
    const diag::Transport transport{diag::TransportOps{memorySend, memoryReceive}, &transportState};
    const std::array<std::uint8_t, 2U> payload{0x11U, 0x22U};
    std::array<std::uint8_t, 4U>       receiveBuffer{};
    std::size_t                        bytesRead = 0U;

    EXPECT_EQ(diag::transportSend(transport, payload.data(), payload.size()), diag::Result::Ok);
    EXPECT_EQ(transportState.sendCalls, 1U);
    EXPECT_EQ(transportState.lastSentSize, payload.size());
    EXPECT_EQ(transportState.lastSent[0], 0x11U);
    EXPECT_EQ(transportState.lastSent[1], 0x22U);

    EXPECT_EQ(
        diag::transportReceive(transport, receiveBuffer.data(), receiveBuffer.size(), bytesRead),
        diag::Result::Ok);
    EXPECT_EQ(transportState.receiveCalls, 1U);
    EXPECT_EQ(bytesRead, 1U);
    EXPECT_EQ(receiveBuffer[0], 0xABU);
}

TEST(DiagTransport, ReceiveRejectsAdapterReportedOverflow)
{
    MemoryTransport              transportState{};
    std::array<std::uint8_t, 4U> buffer{};
    std::size_t                  bytesRead = 7U;
    const diag::Transport        transport{diag::TransportOps{memorySend, oversizedReceive},
                                    &transportState};

    EXPECT_EQ(diag::transportReceive(transport, buffer.data(), buffer.size(), bytesRead),
              diag::Result::Transport);
    EXPECT_EQ(bytesRead, 0U);
}
