#ifndef SOCKETCAN_TRANSPORT_SOCKETCAN_DIAG_LINK_HPP
#define SOCKETCAN_TRANSPORT_SOCKETCAN_DIAG_LINK_HPP

#include "example_diag_tool.h"

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

    bool take(struct example_diag_frame &outFrame);

  private:
    std::uint32_t             m_acceptedId;
    bool                      m_hasFrame = false;
    struct example_diag_frame m_frame = {{0}, 0u};
};

bool receiveFrame(SocketCAN::CanFD &bus, FdFrameInbox &inbox, struct example_diag_frame &outFrame,
                  std::chrono::milliseconds timeout);
void sendFrame(SocketCAN::CanFD &bus, std::uint32_t canId, const struct example_diag_frame &frame);

} // namespace diag_socketcan

#endif
