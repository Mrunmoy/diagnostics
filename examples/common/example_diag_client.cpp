#include "example_diag_client.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace example_diag_client
{
namespace
{

const char *resultName(enum diag_result result)
{
    switch (result)
    {
        case DIAG_OK:
            return "DIAG_OK";
        case DIAG_ERROR_INVALID_ARGUMENT:
            return "DIAG_ERROR_INVALID_ARGUMENT";
        case DIAG_ERROR_NOT_INITIALIZED:
            return "DIAG_ERROR_NOT_INITIALIZED";
        case DIAG_ERROR_NOT_FOUND:
            return "DIAG_ERROR_NOT_FOUND";
        case DIAG_ERROR_ALREADY_EXISTS:
            return "DIAG_ERROR_ALREADY_EXISTS";
        case DIAG_ERROR_CAPACITY:
            return "DIAG_ERROR_CAPACITY";
        case DIAG_ERROR_STORAGE:
            return "DIAG_ERROR_STORAGE";
        case DIAG_ERROR_TRANSPORT:
            return "DIAG_ERROR_TRANSPORT";
        case DIAG_ERROR_CORRUPT_DATA:
            return "DIAG_ERROR_CORRUPT_DATA";
        case DIAG_ERROR_NOT_SUPPORTED:
            return "DIAG_ERROR_NOT_SUPPORTED";
        default:
            return "DIAG_ERROR_UNKNOWN";
    }
}

enum diag_result exchangeChecked(Transport &transport, const char *toolName,
                                 const struct example_diag_frame &request,
                                 struct example_diag_frame &response, std::ostream &err)
{
    enum diag_result result = transport.exchange(request, response);

    if (result != DIAG_OK)
    {
        err << toolName << ": request 0x" << std::hex << static_cast<int>(request.bytes[0])
            << std::dec << " failed: " << resultName(result) << "\n";
        return result;
    }

    if (response.size == 0u)
    {
        err << toolName << ": empty response\n";
        return DIAG_ERROR_CORRUPT_DATA;
    }

    result = static_cast<enum diag_result>(response.bytes[0]);
    if (result != DIAG_OK)
    {
        err << toolName << ": device rejected request 0x" << std::hex
            << static_cast<int>(request.bytes[0]) << std::dec << ": " << resultName(result) << "\n";
    }

    return result;
}

enum diag_result readIdentity(Transport &transport, const char *toolName, std::ostream &out,
                              std::ostream &err)
{
    struct example_diag_frame request = {{0}, 1u};
    struct example_diag_frame response = {{0}, 0u};
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_READ_IDENTITY;
    result = exchangeChecked(transport, toolName, request, response, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size != 11u)
    {
        err << toolName << ": malformed identity response\n";
        return DIAG_ERROR_CORRUPT_DATA;
    }

    const std::uint16_t ecosystem =
        static_cast<std::uint16_t>(response.bytes[1] | (response.bytes[2] << 8u));
    const std::uint16_t product =
        static_cast<std::uint16_t>(response.bytes[3] | (response.bytes[4] << 8u));
    const std::uint16_t deviceType =
        static_cast<std::uint16_t>(response.bytes[5] | (response.bytes[6] << 8u));

    out << toolName << ": identity ecosystem=" << ecosystem << " product=" << product
        << " type=" << deviceType << " instance=" << static_cast<int>(response.bytes[7])
        << " stage=" << static_cast<int>(response.bytes[8])
        << " component=" << static_cast<int>(response.bytes[9]) << "\n";

    return DIAG_OK;
}

enum diag_result listDtcs(Transport &transport, const char *toolName, diag_dtc_id_t clearTarget,
                          std::uint8_t &outClearTargetStatus, std::ostream &out, std::ostream &err)
{
    struct example_diag_frame request = {{0}, 1u};
    struct example_diag_frame response = {{0}, 0u};
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_LIST_DTCS;
    result = exchangeChecked(transport, toolName, request, response, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size < 2u)
    {
        err << toolName << ": malformed DTC response\n";
        return DIAG_ERROR_CORRUPT_DATA;
    }

    const std::size_t count = response.bytes[1];
    if (response.size != (2u + (count * EXAMPLE_DIAG_DTC_WIRE_SIZE)))
    {
        err << toolName << ": DTC response length mismatch\n";
        return DIAG_ERROR_CORRUPT_DATA;
    }

    out << toolName << ": DTC count=" << count << "\n";
    for (std::size_t i = 0u; i < count; ++i)
    {
        const std::uint8_t *record = &response.bytes[2u + (i * EXAMPLE_DIAG_DTC_WIRE_SIZE)];
        const diag_dtc_id_t id = example_diag_read_u32_le(&record[0]);
        const std::uint8_t  status = record[4];
        const std::uint8_t  severity = record[5];
        const std::uint32_t occurrences = example_diag_read_u32_le(&record[6]);

        if (id == clearTarget)
        {
            outClearTargetStatus = status;
        }

        out << toolName << ": DTC 0x" << std::hex << std::setw(6) << std::setfill('0') << id
            << std::setfill(' ') << std::dec << " status=0x" << std::hex << std::setw(2)
            << std::setfill('0') << static_cast<int>(status) << std::setfill(' ') << std::dec
            << " severity=" << static_cast<int>(severity) << " occurrences=" << occurrences << "\n";
    }

    return DIAG_OK;
}

enum diag_result clearDtc(Transport &transport, const char *toolName, diag_dtc_id_t dtcId,
                          std::ostream &out, std::ostream &err)
{
    struct example_diag_frame request = {{0}, 5u};
    struct example_diag_frame response = {{0}, 0u};
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_CLEAR_DTC;
    example_diag_write_u32_le(&request.bytes[1], dtcId);

    result = exchangeChecked(transport, toolName, request, response, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    out << toolName << ": cleared DTC 0x" << std::hex << std::setw(6) << std::setfill('0') << dtcId
        << std::setfill(' ') << std::dec << "\n";

    return DIAG_OK;
}

} // namespace

enum diag_result runDiagnosticSession(Transport &transport, const char *toolName,
                                      const char *linkName, diag_dtc_id_t clearTarget,
                                      std::ostream &out, std::ostream &err)
{
    std::uint8_t     statusBeforeClear = 0xFFu;
    std::uint8_t     statusAfterClear = 0xFFu;
    enum diag_result result = DIAG_OK;

    if (toolName == nullptr || linkName == nullptr)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    out << toolName << ": opening diagnostic session on " << linkName << "\n";

    result = readIdentity(transport, toolName, out, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = listDtcs(transport, toolName, clearTarget, statusBeforeClear, out, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = clearDtc(transport, toolName, clearTarget, out, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = listDtcs(transport, toolName, clearTarget, statusAfterClear, out, err);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (statusBeforeClear == 0u || statusAfterClear != 0u)
    {
        err << toolName << ": clear verification failed\n";
        return DIAG_ERROR_CORRUPT_DATA;
    }

    out << toolName << ": diagnostic session complete\n";
    return DIAG_OK;
}

} // namespace example_diag_client
