#include "socketcan_diag_link.hpp"

#include <algorithm>
#include <cstring>

namespace diag_socketcan
{

FdFrameInbox::FdFrameInbox(std::uint32_t acceptedId) : m_acceptedId(acceptedId)
{
}

void FdFrameInbox::onFdFrame(const SocketCAN::FdFrame &frame)
{
    if (frame.id != m_acceptedId || frame.extended || frame.len > EXAMPLE_DIAG_MAX_FRAME_SIZE)
    {
        return;
    }

    m_frame.size = frame.len;
    std::copy(frame.data.begin(), frame.data.begin() + frame.len, m_frame.bytes);
    m_hasFrame = true;
}

bool FdFrameInbox::take(struct example_diag_frame &outFrame)
{
    if (!m_hasFrame)
    {
        return false;
    }

    outFrame = m_frame;
    m_hasFrame = false;
    return true;
}

bool receiveFrame(SocketCAN::CanFD &bus, FdFrameInbox &inbox, struct example_diag_frame &outFrame,
                  std::chrono::milliseconds timeout)
{
    const auto startedAt = std::chrono::steady_clock::now();

    while (std::chrono::steady_clock::now() - startedAt < timeout)
    {
        if (inbox.take(outFrame))
        {
            return true;
        }

        (void)bus.receive(std::chrono::milliseconds{25});
    }

    return inbox.take(outFrame);
}

void sendFrame(SocketCAN::CanFD &bus, std::uint32_t canId, const struct example_diag_frame &frame)
{
    SocketCAN::FdFrame canFrame;

    if (frame.size > canFrame.data.size())
    {
        throw SocketCAN::CanError("diagnostic frame does not fit in one CAN FD frame");
    }

    canFrame.id = canId;
    canFrame.extended = false;
    canFrame.brs = false;
    canFrame.esi = false;
    canFrame.len = static_cast<std::uint8_t>(frame.size);
    std::copy(frame.bytes, frame.bytes + frame.size, canFrame.data.begin());

    bus.send(canFrame);
}

} // namespace diag_socketcan
