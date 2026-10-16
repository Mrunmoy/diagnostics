#include "example_diag_client.hpp"
#include "example_diag_tool.hpp"
#include "serial_diag_link.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace
{

class SerialTransport final : public example_diag_client::Transport
{
  public:
    explicit SerialTransport(const std::string &path) : m_port(path) {}

    [[nodiscard]] diag::Result exchange(const example_diag::Frame &request,
                                        example_diag::Frame       &response) override
    {
        diag_serial::sendFrame(m_port, request);
        if (!diag_serial::receiveFrame(m_port, response, std::chrono::milliseconds{1000}))
        {
            return diag::Result::Transport;
        }

        if (response.size == 0U)
        {
            return diag::Result::CorruptData;
        }

        return diag::Result::Ok;
    }

  private:
    diag_serial::SerialPort m_port;
};

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " --port PATH\n";
}

bool parseArgs(int argc, char **argv, std::string &port)
{
    int i = 1;
    while (i < argc)
    {
        const std::string arg = argv[i];
        if (arg == "--port" && (i + 1) < argc)
        {
            port = argv[i + 1];
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
    std::string port;
    if (!parseArgs(argc, argv, port))
    {
        printUsage(argv[0]);
        return 1;
    }

    try
    {
        SerialTransport    transport(port);
        const diag::Result result = example_diag_client::runDiagnosticSession(
            transport, "serial_tester", port.c_str(), diag::DtcId{0x030101U}, std::cout, std::cerr);
        if (result != diag::Result::Ok)
        {
            return 1;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "serial_tester: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
