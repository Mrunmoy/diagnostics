#pragma once

#include "example_diag_tool.hpp"

#include <iosfwd>

namespace grafana_reader
{

[[nodiscard]] diag::Result exportPrometheus(std::ostream                     &stream,
                                            const example_diag::ToolSnapshot &snapshot);
[[nodiscard]] diag::Result exportJson(std::ostream                     &stream,
                                      const example_diag::ToolSnapshot &snapshot);

} // namespace grafana_reader
