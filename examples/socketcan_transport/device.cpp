#include "device.hpp"

#include "example_diag_tool.hpp"
#include "socketcan_diag_link.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace
{

struct Options
{
    std::string interface {
        "vcan0"
    };
    std::uint32_t requestLimit{0U};
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " [--interface IFACE] [--count N]\n";
}

bool parseU32(const char *text, std::uint32_t &outValue)
{
    char               *end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 10);
    if (text == nullptr || *text == '\0' || *end != '\0' || value > UINT32_MAX)
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
        diagnostic_session::DiagnosticDevice device{};
        if (device.setup() != diag::Result::Ok)
        {
            std::cerr << "socketcan_device: device setup failed\n";
            return 1;
        }

        const example_diag::Device   endpoint{"socketcan_device", &device.context(),
                                            diagnostic_session::persistedSizeCallback, &device};
        SocketCAN::CanFD             bus{options.interface};
        diag_socketcan::FdFrameInbox inbox{diag_socketcan::kRequestCanId};
        std::uint32_t                handled = 0U;

        bus.addFdListener(&inbox);
        std::cout << "socketcan_device: listening on " << options.interface << "\n";

        while (options.requestLimit == 0U || handled < options.requestLimit)
        {
            example_diag::Frame request{};
            example_diag::Frame response{};
            if (!diag_socketcan::receiveFrame(bus, inbox, request, std::chrono::milliseconds{1000}))
            {
                continue;
            }

            const diag::Result result = example_diag::handleRequest(endpoint, request, response);
            if (result != diag::Result::Ok && response.size == 0U)
            {
                response.bytes[0] = static_cast<std::uint8_t>(result);
                response.size = 1U;
            }

            diag_socketcan::sendFrame(bus, diag_socketcan::kResponseCanId, response);
            ++handled;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "socketcan_device: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
