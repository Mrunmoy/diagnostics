#include "device.h"
#include "exporter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "usage: %s [--demo|--prometheus|--json] [--scenario-step N]\n", program);
}

static int parse_step(const char *text, uint32_t *out_step)
{
    char         *end = NULL;
    unsigned long value = 0u;

    if (text == NULL || out_step == NULL)
    {
        return 0;
    }

    value = strtoul(text, &end, 10);
    if (*text == '\0' || *end != '\0' || value > UINT32_MAX)
    {
        return 0;
    }

    *out_step = (uint32_t)value;
    return 1;
}

int main(int argc, char **argv)
{
    struct grafana_reader_device      device = {0};
    struct example_diag_device        endpoint = {0};
    struct example_diag_tool_snapshot snapshot = {0};
    const char                       *mode = "--demo";
    enum diag_result                  result = DIAG_OK;
    uint32_t                          scenario_step = 0u;
    int                               use_scenario_step = 0;

    if (argc != 1 && argc != 2 && argc != 4)
    {
        print_usage(argv[0]);
        return 1;
    }

    if (argc >= 2)
    {
        mode = argv[1];
    }

    if (argc == 4)
    {
        if (strcmp(argv[2], "--scenario-step") != 0 || !parse_step(argv[3], &scenario_step))
        {
            print_usage(argv[0]);
            return 1;
        }
        use_scenario_step = 1;
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

    if (use_scenario_step)
    {
        result = grafana_reader_device_apply_scenario(&device, scenario_step);
        if (result != DIAG_OK)
        {
            fprintf(stderr, "grafana_reader: scenario step failed: %d\n", (int)result);
            (void)grafana_reader_device_deinit(&device);
            return 1;
        }
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
