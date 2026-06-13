#include "device.h"
#include "exporter.h"

#include <stdio.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "usage: %s [--demo|--prometheus|--json]\n", program);
}

int main(int argc, char **argv)
{
    struct grafana_reader_device      device = {0};
    struct example_diag_device        endpoint = {0};
    struct example_diag_tool_snapshot snapshot = {0};
    const char                       *mode = "--demo";
    enum diag_result                  result = DIAG_OK;

    if (argc > 2)
    {
        print_usage(argv[0]);
        return 1;
    }

    if (argc == 2)
    {
        mode = argv[1];
    }

    if (strcmp(mode, "--demo") != 0 && strcmp(mode, "--prometheus") != 0 &&
        strcmp(mode, "--json") != 0)
    {
        print_usage(argv[0]);
        return 1;
    }

    result = grafana_reader_device_init(&device);
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

    if (strcmp(mode, "--prometheus") == 0)
    {
        result = grafana_reader_export_prometheus(stdout, &snapshot);
    }
    else if (strcmp(mode, "--json") == 0)
    {
        result = grafana_reader_export_json(stdout, &snapshot);
    }
    else if (strcmp(mode, "--demo") == 0)
    {
        printf("grafana_reader: prometheus metrics\n");
        result = grafana_reader_export_prometheus(stdout, &snapshot);
        if (result == DIAG_OK)
        {
            printf("grafana_reader: json snapshot\n");
            result = grafana_reader_export_json(stdout, &snapshot);
        }
    }
    if (result != DIAG_OK)
    {
        fprintf(stderr, "grafana_reader: export failed: %d\n", (int)result);
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
