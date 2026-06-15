#include "example_diag_client.hpp"
#include "example_diag_tool.h"

#include "modbus_rtu_diag_link.hpp"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

namespace
{

class ModbusRtuTransport final : public example_diag_client::Transport
{
  public:
    ModbusRtuTransport(const std::string &path, std::uint8_t slaveAddress)
        : m_port(path), m_slaveAddress(slaveAddress)
    {
    }

    enum diag_result exchange(const struct example_diag_frame &request,
                              struct example_diag_frame       &response) override
    {
        diag_modbus_rtu::sendRequest(m_port, m_slaveAddress, request);
        if (!diag_modbus_rtu::receiveResponse(m_port, m_slaveAddress, response,
                                              std::chrono::milliseconds{1000}))
        {
            return DIAG_ERROR_TRANSPORT;
        }

        if (response.size == 0u)
        {
            return DIAG_ERROR_CORRUPT_DATA;
        }

        return DIAG_OK;
    }

  private:
    diag_serial::SerialPort m_port;
    std::uint8_t            m_slaveAddress;
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " --port PATH [--address N]\n";
}

bool parseU32(const char *text, std::uint32_t &outValue)
{
    char         *end = nullptr;
    unsigned long value = 0u;

    if (text == nullptr || *text == '\0')
    {
        return false;
    }

    value = std::strtoul(text, &end, 0);
    if (*end != '\0' || value > UINT32_MAX)
    {
        return false;
    }

    outValue = static_cast<std::uint32_t>(value);
    return true;
}

bool parseArgs(int argc, char **argv, std::string &port, std::uint8_t &slaveAddress)
{
    int i = 1;

    while (i < argc)
    {
        const std::string arg = argv[i];
        std::uint32_t     value = 0u;

        if (arg == "--port" && (i + 1) < argc)
        {
            port = argv[i + 1];
            i += 2;
        }
        else if (arg == "--address" && (i + 1) < argc && parseU32(argv[i + 1], value) &&
                 value <= 247u && value != 0u)
        {
            slaveAddress = static_cast<std::uint8_t>(value);
            i += 2;
        }
        else
        {
            return false;
        }
    }

    return !port.empty();
}

} // namespace

int main(int argc, char **argv)
{
    std::string             port;
    std::uint8_t            slaveAddress = diag_modbus_rtu::kDefaultSlaveAddress;
    constexpr diag_dtc_id_t clearTarget = 0x030101u;

    if (!parseArgs(argc, argv, port, slaveAddress))
    {
        printUsage(argv[0]);
        return 1;
    }

    try
    {
        ModbusRtuTransport transport(port, slaveAddress);
        const std::string  linkName =
            port + " address=" + std::to_string(static_cast<int>(slaveAddress));
        enum diag_result result = example_diag_client::runDiagnosticSession(
            transport, "modbus_tester", linkName.c_str(), clearTarget, std::cout, std::cerr);

        if (result != DIAG_OK)
        {
            return 1;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "modbus_tester: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
