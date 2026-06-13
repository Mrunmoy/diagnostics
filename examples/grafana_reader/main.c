#include "device.h"
#include "exporter.h"

#include <stdio.h>

int main(void)
{
    struct grafana_reader_device      device = {0};
    struct example_diag_device        endpoint = {0};
    struct example_diag_tool_snapshot snapshot = {0};
    enum diag_result                  result = grafana_reader_device_init(&device);

    if (result != DIAG_OK)
    {
        fprintf(stderr, "grafana_reader: device setup failed: %d\n", (int)result);
        return 1;
    }

    endpoint.name = "grafana_reader";
    endpoint.ctx = device.ctx;
    endpoint.persisted_size = grafana_reader_device_persisted_size;
    endpoint.user = &device;

    result = example_diag_tool_collect_snapshot(&endpoint, &snapshot);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "grafana_reader: tester snapshot failed: %d\n", (int)result);
        (void)grafana_reader_device_deinit(&device);
        return 1;
    }

    printf("grafana_reader: prometheus metrics\n");
    result = grafana_reader_export_prometheus(stdout, &snapshot);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "grafana_reader: prometheus export failed: %d\n", (int)result);
        (void)grafana_reader_device_deinit(&device);
        return 1;
    }

    printf("grafana_reader: json snapshot\n");
    result = grafana_reader_export_json(stdout, &snapshot);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "grafana_reader: json export failed: %d\n", (int)result);
        (void)grafana_reader_device_deinit(&device);
        return 1;
    }

    result = grafana_reader_device_deinit(&device);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "grafana_reader: device cleanup failed: %d\n", (int)result);
        return 1;
    }

    return 0;
}
