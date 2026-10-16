#ifndef SOCKETCAN_TRANSPORT_SOCKETCAN_DIAG_LINK_HPP
#define SOCKETCAN_TRANSPORT_SOCKETCAN_DIAG_LINK_HPP

#include "example_diag_tool.hpp"

#include "SocketCan.hpp"

#include <chrono>

namespace diag_socketcan
{

constexpr std::uint32_t kRequestCanId = 0x650u;
constexpr std::uint32_t kResponseCanId = 0x651u;

class FdFrameInbox final : public SocketCAN::IFdFrameListener
{
  public:
    explicit FdFrameInbox(std::uint32_t acceptedId);

    void onFdFrame(const SocketCAN::FdFrame &frame) override;

    bool take(example_diag::Frame &outFrame);

  private:
    std::uint32_t       m_acceptedId;
    bool                m_hasFrame = false;
    example_diag::Frame m_frame{};
};

bool receiveFrame(SocketCAN::CanFD &bus, FdFrameInbox &inbox, example_diag::Frame &outFrame,
                  std::chrono::milliseconds timeout);
void sendFrame(SocketCAN::CanFD &bus, std::uint32_t canId, const example_diag::Frame &frame);

} // namespace diag_socketcan

#endif
