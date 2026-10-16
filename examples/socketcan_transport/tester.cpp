#include "example_diag_client.hpp"
#include "example_diag_tool.hpp"
#include "socketcan_diag_link.hpp"

#include <exception>
#include <iostream>
#include <string>

namespace
{

class SocketCanTransport final : public example_diag_client::Transport
{
  public:
    explicit SocketCanTransport(const std::string &interface)
        : m_bus(interface), m_inbox(diag_socketcan::kResponseCanId)
    {
        m_bus.addFdListener(&m_inbox);
    }

    [[nodiscard]] diag::Result exchange(const example_diag::Frame &request,
                                        example_diag::Frame       &response) override
    {
        diag_socketcan::sendFrame(m_bus, diag_socketcan::kRequestCanId, request);
        if (!diag_socketcan::receiveFrame(m_bus, m_inbox, response,
                                          std::chrono::milliseconds{1000}))
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
    SocketCAN::CanFD             m_bus;
    diag_socketcan::FdFrameInbox m_inbox;
};

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

} // namespace

int main(int argc, char **argv)
{
    std::string interface = "vcan0";
    if (!parseArgs(argc, argv, interface))
    {
        printUsage(argv[0]);
        return 1;
    }

    try
    {
        SocketCanTransport transport(interface);
        const diag::Result result = example_diag_client::runDiagnosticSession(
            transport, "socketcan_tester", interface.c_str(), diag::DtcId{0x030101U}, std::cout,
            std::cerr);
        if (result != diag::Result::Ok)
        {
            return 1;
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "socketcan_tester: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
