#include "example_diag_client.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace example_diag_client
{
namespace
{

const char *resultName(const diag::Result result)
{
    switch (result)
    {
    case diag::Result::Ok:
        return "Ok";
    case diag::Result::InvalidArgument:
        return "InvalidArgument";
    case diag::Result::NotInitialized:
        return "NotInitialized";
    case diag::Result::NotFound:
        return "NotFound";
    case diag::Result::AlreadyExists:
        return "AlreadyExists";
    case diag::Result::Capacity:
        return "Capacity";
    case diag::Result::Storage:
        return "Storage";
    case diag::Result::Transport:
        return "Transport";
    case diag::Result::CorruptData:
        return "CorruptData";
    case diag::Result::NotSupported:
        return "NotSupported";
    default:
        return "Unknown";
    }
}

[[nodiscard]] diag::Result exchangeChecked(Transport &transport, const char *toolName,
                                           const example_diag::Frame &request,
                                           example_diag::Frame &response, std::ostream &err)
{
    diag::Result result = transport.exchange(request, response);
    if (result != diag::Result::Ok)
    {
        err << toolName << ": request 0x" << std::hex << static_cast<int>(request.bytes[0])
            << std::dec << " failed: " << resultName(result) << "\n";
        return result;
    }

    if (response.size == 0U)
    {
        err << toolName << ": empty response\n";
        return diag::Result::CorruptData;
    }

    result = static_cast<diag::Result>(response.bytes[0]);
    if (result != diag::Result::Ok)
    {
        err << toolName << ": device rejected request 0x" << std::hex
            << static_cast<int>(request.bytes[0]) << std::dec << ": " << resultName(result) << "\n";
    }

    return result;
}

[[nodiscard]] diag::Result readIdentity(Transport &transport, const char *toolName,
                                        std::ostream &out, std::ostream &err)
{
    example_diag::Frame request{};
    example_diag::Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(example_diag::Service::ReadIdentity);
    request.size = 1U;

    const diag::Result result = exchangeChecked(transport, toolName, request, response, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size != 11U)
    {
        err << toolName << ": malformed identity response\n";
        return diag::Result::CorruptData;
    }

    const std::uint16_t ecosystem =
        static_cast<std::uint16_t>(response.bytes[1] | (response.bytes[2] << 8U));
    const std::uint16_t product =
        static_cast<std::uint16_t>(response.bytes[3] | (response.bytes[4] << 8U));
    const std::uint16_t deviceType =
        static_cast<std::uint16_t>(response.bytes[5] | (response.bytes[6] << 8U));

    out << toolName << ": identity ecosystem=" << ecosystem << " product=" << product
        << " type=" << deviceType << " instance=" << static_cast<int>(response.bytes[7])
        << " stage=" << static_cast<int>(response.bytes[8])
        << " component=" << static_cast<int>(response.bytes[9]) << "\n";
    return diag::Result::Ok;
}

[[nodiscard]] diag::Result listDtcs(Transport &transport, const char *toolName,
                                    const diag::DtcId clearTarget,
                                    std::uint8_t &outClearTargetStatus, std::ostream &out,
                                    std::ostream &err)
{
    example_diag::Frame request{};
    example_diag::Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(example_diag::Service::ListDtcs);
    request.size = 1U;

    const diag::Result result = exchangeChecked(transport, toolName, request, response, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size < 2U)
    {
        err << toolName << ": malformed DTC response\n";
        return diag::Result::CorruptData;
    }

    const std::size_t count = response.bytes[1];
    if (response.size != (2U + (count * example_diag::kDtcWireSize)))
    {
        err << toolName << ": DTC response length mismatch\n";
        return diag::Result::CorruptData;
    }

    out << toolName << ": DTC count=" << count << "\n";
    for (std::size_t index = 0U; index < count; ++index)
    {
        const std::uint8_t *const record =
            &response.bytes[2U + (index * example_diag::kDtcWireSize)];
        const diag::DtcId   id{example_diag::readU32Le(&record[0])};
        const std::uint8_t  status = record[4];
        const std::uint8_t  severity = record[5];
        const std::uint32_t occurrences = example_diag::readU32Le(&record[6]);

        if (id == clearTarget)
        {
            outClearTargetStatus = status;
        }

        out << toolName << ": DTC 0x" << std::hex << std::setw(6) << std::setfill('0') << id.value
            << std::setfill(' ') << std::dec << " status=0x" << std::hex << std::setw(2)
            << std::setfill('0') << static_cast<int>(status) << std::setfill(' ') << std::dec
            << " severity=" << static_cast<int>(severity) << " occurrences=" << occurrences << "\n";
    }

    return diag::Result::Ok;
}

[[nodiscard]] diag::Result clearDtc(Transport &transport, const char *toolName,
                                    const diag::DtcId dtcId, std::ostream &out, std::ostream &err)
{
    example_diag::Frame request{};
    example_diag::Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(example_diag::Service::ClearDtc);
    example_diag::writeU32Le(&request.bytes[1], dtcId.value);
    request.size = 5U;

    const diag::Result result = exchangeChecked(transport, toolName, request, response, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    out << toolName << ": cleared DTC 0x" << std::hex << std::setw(6) << std::setfill('0')
        << dtcId.value << std::setfill(' ') << std::dec << "\n";
    return diag::Result::Ok;
}

} // namespace

diag::Result runDiagnosticSession(Transport &transport, const char *const toolName,
                                  const char *const linkName, const diag::DtcId clearTarget,
                                  std::ostream &out, std::ostream &err)
{
    if (toolName == nullptr || linkName == nullptr)
    {
        return diag::Result::InvalidArgument;
    }

    std::uint8_t statusBeforeClear = 0xFFU;
    std::uint8_t statusAfterClear = 0xFFU;

    out << toolName << ": opening diagnostic session on " << linkName << "\n";

    diag::Result result = readIdentity(transport, toolName, out, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = listDtcs(transport, toolName, clearTarget, statusBeforeClear, out, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = clearDtc(transport, toolName, clearTarget, out, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = listDtcs(transport, toolName, clearTarget, statusAfterClear, out, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (statusBeforeClear == 0U || statusAfterClear != 0U)
    {
        err << toolName << ": clear verification failed\n";
        return diag::Result::CorruptData;
    }

    out << toolName << ": diagnostic session complete\n";
    return diag::Result::Ok;
}

} // namespace example_diag_client
