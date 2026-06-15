#include "device.h"
#include "example_diag_tool.h"

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
    std::uint32_t requestLimit = 0u;
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " --port PATH [--count N]\n";
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
        struct diagnostic_device device = {0};
        enum diag_result         result = diagnostic_device_init(&device);

        if (result != DIAG_OK)
        {
            std::cerr << "serial_device: device setup failed: " << static_cast<int>(result) << "\n";
            return 1;
        }

        const struct example_diag_device endpoint = {
            "serial_device",
            device.ctx,
            diagnostic_device_persisted_size,
            &device,
        };

        diag_serial::SerialPort port(options.port);
        std::uint32_t           handled = 0u;

        std::cout << "serial_device: listening on " << options.port << "\n";

        while (options.requestLimit == 0u || handled < options.requestLimit)
        {
            struct example_diag_frame request = {{0}, 0u};
            struct example_diag_frame response = {{0}, 0u};

            if (!diag_serial::receiveFrame(port, request, std::chrono::milliseconds{1000}))
            {
                continue;
            }

            result = example_diag_device_handle_request(&endpoint, &request, &response);
            if (result != DIAG_OK && response.size == 0u)
            {
                response.bytes[0] = static_cast<std::uint8_t>(result);
                response.size = 1u;
            }

            diag_serial::sendFrame(port, response);
            ++handled;
        }

        result = diagnostic_device_deinit(&device);
        if (result != DIAG_OK)
        {
            std::cerr << "serial_device: device cleanup failed: " << static_cast<int>(result)
                      << "\n";
            return 1;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "serial_device: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
