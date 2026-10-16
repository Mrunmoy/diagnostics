#pragma once

#include "example_diag_tool.hpp"

#include <iosfwd>

namespace example_diag_client
{

class Transport
{
  public:
    virtual ~Transport() = default;

    [[nodiscard]] virtual diag::Result exchange(const example_diag::Frame &request,
                                                example_diag::Frame       &response) = 0;
};

[[nodiscard]] diag::Result runDiagnosticSession(Transport &transport, const char *toolName,
                                                const char *linkName, diag::DtcId clearTarget,
                                                std::ostream &out, std::ostream &err);

} // namespace example_diag_client
