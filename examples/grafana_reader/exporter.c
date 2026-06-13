#include "exporter.h"

static unsigned int status_bit_is_set(uint8_t status, uint8_t mask)
{
    return (status & mask) != 0u ? 1u : 0u;
}

// clang-format off
enum diag_result grafana_reader_export_prometheus(
    FILE *stream,
    const struct example_diag_tool_snapshot *snapshot)
// clang-format on
{
    size_t i = 0u;

    if (stream == NULL || snapshot == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    fprintf(stream, "# HELP diag_tester_up Diagnostic tester scrape health.\n");
    fprintf(stream, "# TYPE diag_tester_up gauge\n");
    fprintf(stream, "diag_tester_up 1\n");
    fprintf(stream, "# HELP diag_device_identity_info Compact diagnostic identity.\n");
    fprintf(stream, "# TYPE diag_device_identity_info gauge\n");
    fprintf(stream,
            "diag_device_identity_info{ecosystem_id=\"%u\",product_id=\"%u\","
            "device_type=\"%u\",instance=\"%u\",stage=\"%u\",component=\"%u\"} 1\n",
            (unsigned int)snapshot->identity.ecosystem_id,
            (unsigned int)snapshot->identity.product_id,
            (unsigned int)snapshot->identity.device_type,
            (unsigned int)snapshot->identity.device_instance,
            (unsigned int)snapshot->identity.firmware_stage,
            (unsigned int)snapshot->identity.firmware_component);
    fprintf(stream, "# HELP diag_dtc_registered_info Registered DTC identity and severity.\n");
    fprintf(stream, "# TYPE diag_dtc_registered_info gauge\n");
    fprintf(stream, "# HELP diag_dtc_active DTC test_failed status bit.\n");
    fprintf(stream, "# TYPE diag_dtc_active gauge\n");
    fprintf(stream, "# HELP diag_dtc_confirmed DTC confirmed status bit.\n");
    fprintf(stream, "# TYPE diag_dtc_confirmed gauge\n");
    fprintf(stream, "# HELP diag_dtc_occurrences_total DTC occurrence counter.\n");
    fprintf(stream, "# TYPE diag_dtc_occurrences_total counter\n");

    for (i = 0u; i < snapshot->dtc_count; ++i)
    {
        const struct example_diag_dtc_snapshot *dtc = &snapshot->dtcs[i];

        fprintf(stream, "diag_dtc_registered_info{dtc_id=\"0x%06lx\",severity=\"%u\"} 1\n",
                (unsigned long)dtc->id, (unsigned int)dtc->severity);
        fprintf(stream, "diag_dtc_active{dtc_id=\"0x%06lx\"} %u\n", (unsigned long)dtc->id,
                status_bit_is_set(dtc->status, DIAG_DTC_STATUS_TEST_FAILED));
        fprintf(stream, "diag_dtc_confirmed{dtc_id=\"0x%06lx\"} %u\n", (unsigned long)dtc->id,
                status_bit_is_set(dtc->status, DIAG_DTC_STATUS_CONFIRMED));
        fprintf(stream, "diag_dtc_occurrences_total{dtc_id=\"0x%06lx\"} %lu\n",
                (unsigned long)dtc->id, (unsigned long)dtc->occurrence_count);
    }

    fprintf(stream, "# HELP diag_capsule_persisted_bytes Stored diagnostic capsule size.\n");
    fprintf(stream, "# TYPE diag_capsule_persisted_bytes gauge\n");
    fprintf(stream, "diag_capsule_persisted_bytes %lu\n", (unsigned long)snapshot->persisted_size);

    return DIAG_OK;
}

// clang-format off
enum diag_result grafana_reader_export_json(
    FILE *stream,
    const struct example_diag_tool_snapshot *snapshot)
// clang-format on
{
    size_t i = 0u;

    if (stream == NULL || snapshot == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    fprintf(stream, "{\n");
    fprintf(stream, "  \"identity\": {\n");
    fprintf(stream, "    \"ecosystem_id\": %u,\n", (unsigned int)snapshot->identity.ecosystem_id);
    fprintf(stream, "    \"product_id\": %u,\n", (unsigned int)snapshot->identity.product_id);
    fprintf(stream, "    \"device_type\": %u,\n", (unsigned int)snapshot->identity.device_type);
    fprintf(stream, "    \"instance\": %u,\n", (unsigned int)snapshot->identity.device_instance);
    fprintf(stream, "    \"stage\": %u,\n", (unsigned int)snapshot->identity.firmware_stage);
    fprintf(stream, "    \"component\": %u\n", (unsigned int)snapshot->identity.firmware_component);
    fprintf(stream, "  },\n");
    fprintf(stream, "  \"dtcs\": [\n");

    for (i = 0u; i < snapshot->dtc_count; ++i)
    {
        const struct example_diag_dtc_snapshot *dtc = &snapshot->dtcs[i];
        const char                             *suffix = (i + 1u) == snapshot->dtc_count ? "" : ",";

        fprintf(stream,
                "    {\"id\": \"0x%06lx\", \"status\": %u, \"severity\": %u, "
                "\"occurrences\": %lu}%s\n",
                (unsigned long)dtc->id, (unsigned int)dtc->status, (unsigned int)dtc->severity,
                (unsigned long)dtc->occurrence_count, suffix);
    }

    fprintf(stream, "  ],\n");
    fprintf(stream, "  \"persisted_capsule_bytes\": %lu\n",
            (unsigned long)snapshot->persisted_size);
    fprintf(stream, "}\n");

    return DIAG_OK;
}
