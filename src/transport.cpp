#include "diag/transport.hpp"

namespace diag
{

Result validateTransport(const Transport &transport) noexcept
{
    if (transport.ops.send == nullptr || transport.ops.receive == nullptr)
    {
        return Result::InvalidArgument;
    }

    return Result::Ok;
}

Result transportSend(const Transport &transport, const std::uint8_t *const buffer,
                     const std::size_t size) noexcept
{
    const Result validation = validateTransport(transport);
    if (validation != Result::Ok)
    {
        return validation;
    }

    if (buffer == nullptr || size == 0U)
    {
        return Result::InvalidArgument;
    }

    return transport.ops.send(transport.user, buffer, size);
}

Result transportReceive(const Transport &transport, std::uint8_t *const buffer,
                        const std::size_t bufferSize, std::size_t &bytesRead) noexcept
{
    const Result validation = validateTransport(transport);
    bytesRead = 0U;

    if (validation != Result::Ok)
    {
        return validation;
    }

    if (buffer == nullptr || bufferSize == 0U)
    {
        return Result::InvalidArgument;
    }

    const Result result = transport.ops.receive(transport.user, buffer, bufferSize, bytesRead);
    if (result == Result::Ok && bytesRead > bufferSize)
    {
        bytesRead = 0U;
        return Result::Transport;
    }

    return result;
}

} // namespace diag
