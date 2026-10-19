#include "diag/diag.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

struct IoBus
{
    std::uint8_t lastFrame[16]{};
    std::size_t  lastFrameSize{0U};
};

diag::Result ioSend(void *const user, const std::uint8_t *const buffer,
                    const std::size_t size) noexcept
{
    IoBus &bus = *static_cast<IoBus *>(user);
    if (size > sizeof(bus.lastFrame))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(bus.lastFrame, buffer, size);
    bus.lastFrameSize = size;
    return diag::Result::Ok;
}

diag::Result ioReceive(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                       std::size_t &bytesRead) noexcept
{
    IoBus &bus = *static_cast<IoBus *>(user);
    if (bufferSize < bus.lastFrameSize)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, bus.lastFrame, bus.lastFrameSize);
    bytesRead = bus.lastFrameSize;
    return diag::Result::Ok;
}

} // namespace

int main()
{
    IoBus                bus{};
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    std::uint8_t         rx[16]{};
    std::size_t          rxSize = 0U;
    const std::uint8_t   request[] = {0x22U, 0x10U, 0x01U};

    const diag::Identity  identity{diag::EcosystemId{1U},   diag::ProductId{40U},
                                  diag::DeviceType{4U},    diag::DeviceInstance{3U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{1U}};
    const diag::Transport transport{diag::TransportOps{ioSend, ioReceive}, &bus};

    if (!context.isInitialized() || context.attachIdentity(identity) != diag::Result::Ok ||
        context.attachTransport(transport) != diag::Result::Ok)
    {
        std::cerr << "io_module: setup failed\n";
        return 1;
    }

    if (diag::transportSend(transport, request, sizeof(request)) != diag::Result::Ok ||
        diag::transportReceive(transport, rx, sizeof(rx), rxSize) != diag::Result::Ok ||
        rxSize != sizeof(request))
    {
        std::cerr << "io_module: transport loopback failed\n";
        return 1;
    }

    std::cout << "io_module: looped back " << rxSize << " diagnostic bytes\n";
    return 0;
}
