#include "device.hpp"

#include "example_diag_client.hpp"
#include "example_diag_tool.hpp"

#include <iostream>

namespace
{

class InProcessTransport final : public example_diag_client::Transport
{
  public:
    explicit InProcessTransport(example_diag::Device device) noexcept : m_device{device} {}

    [[nodiscard]] diag::Result exchange(const example_diag::Frame &request,
                                        example_diag::Frame       &response) override
    {
        const diag::Result result = example_diag::handleRequest(m_device, request, response);
        // Encoded negative responses are still successful transport exchanges; the
        // service status lives in response.bytes[0] for the client to decode.
        if (response.size > 0U)
        {
            return diag::Result::Ok;
        }

        return result;
    }

  private:
    example_diag::Device m_device;
};

} // namespace

int main()
{
    diagnostic_session::DiagnosticDevice device{};
    const diag::Result                   setup = device.setup();
    if (setup != diag::Result::Ok)
    {
        std::cerr << "diagnostic_session: device setup failed\n";
        return 1;
    }

    const example_diag::Device endpoint{"diagnostic_session", &device.context(),
                                        diagnostic_session::persistedSizeCallback, &device};
    InProcessTransport         transport{endpoint};

    const diag::Result result =
        example_diag_client::runDiagnosticSession(transport, "diagnostic_session", "in-process",
                                                  diag::DtcId{0x030101U}, std::cout, std::cerr);
    if (result != diag::Result::Ok)
    {
        std::cerr << "diagnostic_session: tester failed\n";
        return 1;
    }

    return 0;
}
