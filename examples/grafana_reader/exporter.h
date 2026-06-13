#ifndef GRAFANA_READER_EXPORTER_H
#define GRAFANA_READER_EXPORTER_H

#include "example_diag_tool.h"

#include <stdio.h>

// clang-format off
enum diag_result grafana_reader_export_prometheus(
    FILE *stream,
    const struct example_diag_tool_snapshot *snapshot);
enum diag_result grafana_reader_export_json(
    FILE *stream,
    const struct example_diag_tool_snapshot *snapshot);
// clang-format on

#endif
