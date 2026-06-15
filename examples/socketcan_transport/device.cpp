#include "device.h"
#include "example_diag_tool.h"

#include "socketcan_diag_link.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

struct Options
{
    std::string   interface = "vcan0";
    std::uint32_t requestLimit = 0u;
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " [--interface IFACE] [--count N]\n";
}

bool parseU32(const char *text, std::uint32_t &outValue)
{
    char         *end = nullptr;
    unsigned long value = 0u;

    if (text == nullptr || *text == '\0')
    {
        return false;
    }

    value = std::strtoul(text, &end, 10);
    if (*end != '\0' || value > UINT32_MAX)
    {
        return false;
    }

    outValue = static_cast<std::uint32_t>(value);
    return true;
}

bool parseArgs(int argc, char **argv, Options &options)
{
    int i = 1;

    while (i < argc)
    {
        const std::string arg = argv[i];

        if (arg == "--interface" && (i + 1) < argc)
        {
            options.interface = argv[i + 1];
            i += 2;
        }
        else if (arg == "--count" && (i + 1) < argc && parseU32(argv[i + 1], options.requestLimit))
        {
            i += 2;
        }
        else
        {
            return false;
        }
    }

    return true;
}

} // namespace

int main(int argc, char **argv)
{
    Options options;

    if (!parseArgs(argc, argv, options))
    {
        printUsage(argv[0]);
        return 1;
    }

    try
    {
        struct diagnostic_device device = {0};
        enum diag_result         result = diagnostic_device_init(&device);

        if (result != DIAG_OK)
        {
            std::cerr << "socketcan_device: device setup failed: " << static_cast<int>(result)
                      << "\n";
            return 1;
        }

        const struct example_diag_device endpoint = {
            "socketcan_device",
            device.ctx,
            diagnostic_device_persisted_size,
            &device,
        };

        SocketCAN::CanFD             bus{options.interface};
        diag_socketcan::FdFrameInbox inbox{diag_socketcan::kRequestCanId};
        std::uint32_t                handled = 0u;

        bus.addFdListener(&inbox);
        std::cout << "socketcan_device: listening on " << options.interface << "\n";

        while (options.requestLimit == 0u || handled < options.requestLimit)
        {
            struct example_diag_frame request = {{0}, 0u};
            struct example_diag_frame response = {{0}, 0u};

            if (!diag_socketcan::receiveFrame(bus, inbox, request, std::chrono::milliseconds{1000}))
            {
                continue;
            }

            result = example_diag_device_handle_request(&endpoint, &request, &response);
            if (result != DIAG_OK && response.size == 0u)
            {
                response.bytes[0] = static_cast<std::uint8_t>(result);
                response.size = 1u;
            }

            diag_socketcan::sendFrame(bus, diag_socketcan::kResponseCanId, response);
            ++handled;
        }

        result = diagnostic_device_deinit(&device);
        if (result != DIAG_OK)
        {
            std::cerr << "socketcan_device: device cleanup failed: " << static_cast<int>(result)
                      << "\n";
            return 1;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "socketcan_device: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
