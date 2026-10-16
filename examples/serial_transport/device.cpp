#include "device.hpp"

#include "example_diag_tool.hpp"
#include "serial_diag_link.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace
{

struct Options
{
    std::string   port;
    std::uint32_t requestLimit{0U};
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " --port PATH [--count N]\n";
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
        if (arg == "--port" && (i + 1) < argc)
        {
            options.port = argv[i + 1];
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

    return !options.port.empty();
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
            std::cerr << "serial_device: device setup failed\n";
            return 1;
        }

        const example_diag::Device endpoint{"serial_device", &device.context(),
                                            diagnostic_session::persistedSizeCallback, &device};
        diag_serial::SerialPort    port(options.port);
        std::uint32_t              handled = 0U;

        std::cout << "serial_device: listening on " << options.port << "\n";
        while (options.requestLimit == 0U || handled < options.requestLimit)
        {
            example_diag::Frame request{};
            example_diag::Frame response{};
            if (!diag_serial::receiveFrame(port, request, std::chrono::milliseconds{1000}))
            {
                continue;
            }

            const diag::Result result = example_diag::handleRequest(endpoint, request, response);
            if (result != diag::Result::Ok && response.size == 0U)
            {
                response.bytes[0] = static_cast<std::uint8_t>(result);
                response.size = 1U;
            }

            diag_serial::sendFrame(port, response);
            ++handled;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "serial_device: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
