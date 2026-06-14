#include "device.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program)
{
    fprintf(stderr, "usage: %s [--json] [--scenario-step N] [--clear DTC_ID]\n", program);
}

static int parse_u32(const char *text, uint32_t *out_value)
{
    char         *end = NULL;
    unsigned long value = 0u;
    int           base = 10;

    if (text == NULL || out_value == NULL)
    {
        return 0;
    }

    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
        base = 16;
    }

    value = strtoul(text, &end, base);
    if (*text == '\0' || *end != '\0' || value > UINT32_MAX)
    {
        return 0;
    }

    *out_value = (uint32_t)value;
    return 1;
}

static unsigned int status_bit_is_set(uint8_t status, uint8_t mask)
{
    return (status & mask) != 0u ? 1u : 0u;
}

static const char *severity_name(uint8_t severity)
{
    switch ((enum diag_dtc_severity)severity)
    {
        case DIAG_DTC_SEVERITY_INFO:
            return "info";
        case DIAG_DTC_SEVERITY_WARNING:
            return "warning";
        case DIAG_DTC_SEVERITY_ERROR:
            return "error";
        default:
            return "unknown";
    }
}

static const char *dtc_title(diag_dtc_id_t id)
{
    switch (id)
    {
        case 0x050101u:
            return "Heater rail over-temperature";
        case 0x050102u:
            return "Cooling fan speed degraded";
        case 0x050103u:
            return "Fieldbus heartbeat missed";
        default:
            return "Unknown diagnostic trouble code";
    }
}

// clang-format off
static void print_json(
    const struct example_diag_tool_snapshot *snapshot,
    uint32_t scenario_step)
// clang-format on
{
    size_t       i = 0u;
    unsigned int active_count = 0u;
    unsigned int confirmed_count = 0u;

    for (i = 0u; i < snapshot->dtc_count; ++i)
    {
        const struct example_diag_dtc_snapshot *dtc = &snapshot->dtcs[i];

        active_count += status_bit_is_set(dtc->status, DIAG_DTC_STATUS_TEST_FAILED);
        confirmed_count += status_bit_is_set(dtc->status, DIAG_DTC_STATUS_CONFIRMED);
    }

    printf("{\n");
    printf("  \"scenario_step\": %lu,\n", (unsigned long)scenario_step);
    printf("  \"summary\": {\n");
    printf("    \"active_dtcs\": %u,\n", active_count);
    printf("    \"confirmed_dtcs\": %u,\n", confirmed_count);
    printf("    \"stored_bytes\": %lu,\n", (unsigned long)snapshot->persisted_size);
    printf("    \"health\": \"%s\"\n", active_count > 0u ? "service_required" : "nominal");
    printf("  },\n");
    printf("  \"identity\": {\n");
    printf("    \"ecosystem_id\": %u,\n", (unsigned int)snapshot->identity.ecosystem_id);
    printf("    \"product_id\": %u,\n", (unsigned int)snapshot->identity.product_id);
    printf("    \"device_type\": %u,\n", (unsigned int)snapshot->identity.device_type);
    printf("    \"device_instance\": %u,\n", (unsigned int)snapshot->identity.device_instance);
    printf("    \"firmware_stage\": %u,\n", (unsigned int)snapshot->identity.firmware_stage);
    printf("    \"firmware_component\": %u\n", (unsigned int)snapshot->identity.firmware_component);
    printf("  },\n");
    printf("  \"dtcs\": [\n");

    for (i = 0u; i < snapshot->dtc_count; ++i)
    {
        const struct example_diag_dtc_snapshot *dtc = &snapshot->dtcs[i];
        const char                             *suffix = (i + 1u) == snapshot->dtc_count ? "" : ",";
        const unsigned int active = status_bit_is_set(dtc->status, DIAG_DTC_STATUS_TEST_FAILED);
        const unsigned int confirmed = status_bit_is_set(dtc->status, DIAG_DTC_STATUS_CONFIRMED);

        printf("    {");
        printf("\"id\":\"0x%06lx\",", (unsigned long)dtc->id);
        printf("\"title\":\"%s\",", dtc_title(dtc->id));
        printf("\"severity\":\"%s\",", severity_name(dtc->severity));
        printf("\"severity_code\":%u,", (unsigned int)dtc->severity);
        printf("\"status\":%u,", (unsigned int)dtc->status);
        printf("\"active\":%s,", active != 0u ? "true" : "false");
        printf("\"confirmed\":%s,", confirmed != 0u ? "true" : "false");
        printf("\"occurrences\":%lu", (unsigned long)dtc->occurrence_count);
        printf("}%s\n", suffix);
    }

    printf("  ]\n");
    printf("}\n");
}

int main(int argc, char **argv)
{
    struct diagnostic_viewer_device   device = {0};
    struct example_diag_device        endpoint = {0};
    struct example_diag_tool_snapshot snapshot = {0};
    enum diag_result                  result = DIAG_OK;
    uint32_t                          scenario_step = 0u;
    uint32_t                          clear_dtc_id = 0u;
    int                               i = 1;

    while (i < argc)
    {
        if (strcmp(argv[i], "--json") == 0)
        {
            ++i;
        }
        else if (strcmp(argv[i], "--scenario-step") == 0 && (i + 1) < argc &&
                 parse_u32(argv[i + 1], &scenario_step))
        {
            i += 2;
        }
        else if (strcmp(argv[i], "--clear") == 0 && (i + 1) < argc &&
                 parse_u32(argv[i + 1], &clear_dtc_id))
        {
            i += 2;
        }
        else
        {
            print_usage(argv[0]);
            return 1;
        }
    }

    result = diagnostic_viewer_device_init(&device);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_viewer: device setup failed: %d\n", (int)result);
        return 1;
    }

    result = diagnostic_viewer_device_apply_scenario(&device, scenario_step);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_viewer: scenario failed: %d\n", (int)result);
        (void)diagnostic_viewer_device_deinit(&device);
        return 1;
    }

    if (clear_dtc_id != 0u)
    {
        result = diagnostic_viewer_device_clear(&device, clear_dtc_id);
        if (result != DIAG_OK)
        {
            fprintf(stderr, "diagnostic_viewer: clear failed for 0x%06lx: %d\n",
                    (unsigned long)clear_dtc_id, (int)result);
            (void)diagnostic_viewer_device_deinit(&device);
            return 1;
        }
    }

    endpoint.name = "diagnostic_viewer";
    endpoint.ctx = device.ctx;
    endpoint.persisted_size = diagnostic_viewer_device_persisted_size;
    endpoint.user = &device;

    result = example_diag_tool_collect_snapshot(&endpoint, &snapshot);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_viewer: snapshot failed: %d\n", (int)result);
        (void)diagnostic_viewer_device_deinit(&device);
        return 1;
    }

    print_json(&snapshot, scenario_step);

    result = diagnostic_viewer_device_deinit(&device);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_viewer: device cleanup failed: %d\n", (int)result);
        return 1;
    }

    return 0;
}
