#ifndef EXAMPLE_DIAG_CLIENT_HPP
#define EXAMPLE_DIAG_CLIENT_HPP

#include "example_diag_tool.h"

#include <iosfwd>

namespace example_diag_client
{

class Transport
{
  public:
    virtual ~Transport() = default;

    virtual enum diag_result exchange(const struct example_diag_frame &request,
                                      struct example_diag_frame       &response) = 0;
};

enum diag_result runDiagnosticSession(Transport &transport, const char *toolName,
                                      const char *linkName, diag_dtc_id_t clearTarget,
                                      std::ostream &out, std::ostream &err);

} // namespace example_diag_client

#endif
