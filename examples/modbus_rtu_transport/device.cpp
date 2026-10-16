#include "device.hpp"

#include "example_diag_tool.hpp"
#include "modbus_rtu_diag_link.hpp"

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
    std::uint8_t  slaveAddress{diag_modbus_rtu::kDefaultSlaveAddress};
    std::uint32_t requestLimit{0U};
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " --port PATH [--address N] [--count N]\n";
}

bool parseU32(const char *text, std::uint32_t &outValue)
{
    char               *end = nullptr;
    const unsigned long value = std::strtoul(text, &end, 0);
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
        std::uint32_t     value = 0U;
        if (arg == "--port" && (i + 1) < argc)
        {
            options.port = argv[i + 1];
            i += 2;
        }
        else if (arg == "--address" && (i + 1) < argc && parseU32(argv[i + 1], value) &&
                 value <= 247U && value != 0U)
        {
            options.slaveAddress = static_cast<std::uint8_t>(value);
            i += 2;
        }
        else if (arg == "--count" && (i + 1) < argc && parseU32(argv[i + 1], value))
        {
            options.requestLimit = value;
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
            std::cerr << "modbus_device: device setup failed\n";
            return 1;
        }

        const example_diag::Device endpoint{"modbus_device", &device.context(),
                                            diagnostic_session::persistedSizeCallback, &device};
        diag_serial::SerialPort    port(options.port);
        std::uint32_t              handled = 0U;

        std::cout << "modbus_device: listening on " << options.port
                  << " address=" << static_cast<int>(options.slaveAddress) << "\n";

        while (options.requestLimit == 0U || handled < options.requestLimit)
        {
            example_diag::Frame request{};
            example_diag::Frame response{};
            if (!diag_modbus_rtu::receiveRequest(port, options.slaveAddress, request,
                                                 std::chrono::milliseconds{1000}))
            {
                continue;
            }

            const diag::Result result = example_diag::handleRequest(endpoint, request, response);
            if (result != diag::Result::Ok && response.size == 0U)
            {
                response.bytes[0] = static_cast<std::uint8_t>(result);
                response.size = 1U;
            }

            diag_modbus_rtu::sendResponse(port, options.slaveAddress, response);
            ++handled;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "modbus_device: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
