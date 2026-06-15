#include "example_diag_tool.h"

#include "socketcan_diag_link.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " [--interface IFACE]\n";
}

bool parseArgs(int argc, char **argv, std::string &interface)
{
    int i = 1;

    while (i < argc)
    {
        const std::string arg = argv[i];

        if (arg == "--interface" && (i + 1) < argc)
        {
            interface = argv[i + 1];
            i += 2;
        }
        else
        {
            return false;
        }
    }

    return true;
}

enum diag_result exchange(SocketCAN::CanFD &bus, diag_socketcan::FdFrameInbox &inbox,
                          const struct example_diag_frame &request,
                          struct example_diag_frame       &response)
{
    diag_socketcan::sendFrame(bus, diag_socketcan::kRequestCanId, request);
    if (!diag_socketcan::receiveFrame(bus, inbox, response, std::chrono::milliseconds{1000}))
    {
        return DIAG_ERROR_TRANSPORT;
    }

    if (response.size == 0u)
    {
        return DIAG_ERROR_CORRUPT_DATA;
    }

    return static_cast<enum diag_result>(response.bytes[0]);
}

enum diag_result readIdentity(SocketCAN::CanFD &bus, diag_socketcan::FdFrameInbox &inbox)
{
    struct example_diag_frame request = {{0}, 1u};
    struct example_diag_frame response = {{0}, 0u};
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_READ_IDENTITY;
    result = exchange(bus, inbox, request, response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size != 11u)
    {
        return DIAG_ERROR_CORRUPT_DATA;
    }

    const std::uint16_t ecosystem =
        static_cast<std::uint16_t>(response.bytes[1] | (response.bytes[2] << 8u));
    const std::uint16_t product =
        static_cast<std::uint16_t>(response.bytes[3] | (response.bytes[4] << 8u));
    const std::uint16_t deviceType =
        static_cast<std::uint16_t>(response.bytes[5] | (response.bytes[6] << 8u));

    std::cout << "socketcan_tester: identity ecosystem=" << ecosystem << " product=" << product
              << " type=" << deviceType << " instance=" << static_cast<int>(response.bytes[7])
              << " stage=" << static_cast<int>(response.bytes[8])
              << " component=" << static_cast<int>(response.bytes[9]) << "\n";

    return DIAG_OK;
}

enum diag_result listDtcs(SocketCAN::CanFD &bus, diag_socketcan::FdFrameInbox &inbox,
                          diag_dtc_id_t clearTarget, std::uint8_t &outClearTargetStatus)
{
    struct example_diag_frame request = {{0}, 1u};
    struct example_diag_frame response = {{0}, 0u};
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_LIST_DTCS;
    result = exchange(bus, inbox, request, response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size < 2u)
    {
        return DIAG_ERROR_CORRUPT_DATA;
    }

    const std::size_t count = response.bytes[1];
    if (response.size != (2u + (count * EXAMPLE_DIAG_DTC_WIRE_SIZE)))
    {
        return DIAG_ERROR_CORRUPT_DATA;
    }

    std::cout << "socketcan_tester: DTC count=" << count << "\n";
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

        std::cout << "socketcan_tester: DTC 0x" << std::hex << id << std::dec << " status=0x"
                  << std::hex << static_cast<int>(status) << std::dec
                  << " severity=" << static_cast<int>(severity) << " occurrences=" << occurrences
                  << "\n";
    }

    return DIAG_OK;
}

enum diag_result clearDtc(SocketCAN::CanFD &bus, diag_socketcan::FdFrameInbox &inbox,
                          diag_dtc_id_t dtcId)
{
    struct example_diag_frame request = {{0}, 5u};
    struct example_diag_frame response = {{0}, 0u};

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_CLEAR_DTC;
    example_diag_write_u32_le(&request.bytes[1], dtcId);

    return exchange(bus, inbox, request, response);
}

} // namespace

int main(int argc, char **argv)
{
    std::string             interface = "vcan0";
    constexpr diag_dtc_id_t clearTarget = 0x030101u;

    if (!parseArgs(argc, argv, interface))
    {
        printUsage(argv[0]);
        return 1;
    }

    try
    {
        SocketCAN::CanFD             bus{interface};
        diag_socketcan::FdFrameInbox inbox{diag_socketcan::kResponseCanId};
        std::uint8_t                 statusBeforeClear = 0xFFu;
        std::uint8_t                 statusAfterClear = 0xFFu;
        enum diag_result             result = DIAG_OK;

        bus.addFdListener(&inbox);
        std::cout << "socketcan_tester: opening diagnostic session on " << interface << "\n";

        result = readIdentity(bus, inbox);
        if (result != DIAG_OK)
        {
            return 1;
        }

        result = listDtcs(bus, inbox, clearTarget, statusBeforeClear);
        if (result != DIAG_OK)
        {
            return 1;
        }

        result = clearDtc(bus, inbox, clearTarget);
        if (result != DIAG_OK)
        {
            return 1;
        }
        std::cout << "socketcan_tester: cleared DTC 0x" << std::hex << clearTarget << std::dec
                  << "\n";

        result = listDtcs(bus, inbox, clearTarget, statusAfterClear);
        if (result != DIAG_OK)
        {
            return 1;
        }

        if (statusBeforeClear == 0u || statusAfterClear != 0u)
        {
            std::cerr << "socketcan_tester: clear verification failed\n";
            return 1;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "socketcan_tester: " << error.what() << "\n";
        return 1;
    }

    std::cout << "socketcan_tester: diagnostic session complete\n";
    return 0;
}
