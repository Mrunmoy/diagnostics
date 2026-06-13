#include "exporter.h"

static unsigned int status_bit_is_set(uint8_t status, uint8_t mask)
{
    return (status & mask) != 0u ? 1u : 0u;
}

static enum diag_result validate_snapshot(const struct example_diag_tool_snapshot *snapshot)
{
    if (snapshot == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (snapshot->dtc_count > EXAMPLE_DIAG_MAX_DTC_SNAPSHOT_COUNT)
    {
        return DIAG_ERROR_CAPACITY;
    }

    return DIAG_OK;
}

// clang-format off
enum diag_result grafana_reader_export_prometheus(
    FILE *stream,
    const struct example_diag_tool_snapshot *snapshot)
// clang-format on
{
    enum diag_result result = DIAG_OK;
    size_t           i = 0u;

    if (stream == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    result = validate_snapshot(snapshot);
    if (result != DIAG_OK)
    {
        return result;
    }

    fprintf(stream, "# HELP diag_tester_up Diagnostic tester scrape health.\n");
    fprintf(stream, "# TYPE diag_tester_up gauge\n");
    fprintf(stream, "diag_tester_up 1\n");
    fprintf(stream, "# HELP diag_device_identity_info Compact diagnostic identity.\n");
    fprintf(stream, "# TYPE diag_device_identity_info gauge\n");
    fprintf(stream,
            "diag_device_identity_info{ecosystem_id=\"%u\",product_id=\"%u\","
            "device_type=\"%u\",device_instance=\"%u\",stage=\"%u\",component=\"%u\"} 1\n",
            (unsigned int)snapshot->identity.ecosystem_id,
            (unsigned int)snapshot->identity.product_id,
            (unsigned int)snapshot->identity.device_type,
            (unsigned int)snapshot->identity.device_instance,
            (unsigned int)snapshot->identity.firmware_stage,
            (unsigned int)snapshot->identity.firmware_component);
    fprintf(stream, "# HELP diag_dtc_registered_info Registered DTC identity and severity.\n");
    fprintf(stream, "# TYPE diag_dtc_registered_info gauge\n");
    fprintf(stream, "# HELP diag_dtc_status_info Dashboard-friendly DTC status labels.\n");
    fprintf(stream, "# TYPE diag_dtc_status_info gauge\n");
    fprintf(stream, "# HELP diag_dtc_active DTC test_failed status bit.\n");
    fprintf(stream, "# TYPE diag_dtc_active gauge\n");
    fprintf(stream, "# HELP diag_dtc_confirmed DTC confirmed status bit.\n");
    fprintf(stream, "# TYPE diag_dtc_confirmed gauge\n");
    fprintf(stream, "# HELP diag_dtc_occurrences DTC occurrence count in the current snapshot.\n");
    fprintf(stream, "# TYPE diag_dtc_occurrences gauge\n");

    for (i = 0u; i < snapshot->dtc_count; ++i)
    {
        const struct example_diag_dtc_snapshot *dtc = &snapshot->dtcs[i];
        const unsigned int active = status_bit_is_set(dtc->status, DIAG_DTC_STATUS_TEST_FAILED);
        const unsigned int confirmed = status_bit_is_set(dtc->status, DIAG_DTC_STATUS_CONFIRMED);

        fprintf(stream, "diag_dtc_registered_info{dtc_id=\"0x%06lx\",severity=\"%u\"} 1\n",
                (unsigned long)dtc->id, (unsigned int)dtc->severity);
        fprintf(stream,
                "diag_dtc_status_info{dtc_id=\"0x%06lx\",status=\"0x%02x\",severity=\"%u\","
                "active=\"%u\",confirmed=\"%u\",occurrences=\"%lu\"} 1\n",
                (unsigned long)dtc->id, (unsigned int)dtc->status, (unsigned int)dtc->severity,
                active, confirmed, (unsigned long)dtc->occurrence_count);
        fprintf(stream, "diag_dtc_active{dtc_id=\"0x%06lx\"} %u\n", (unsigned long)dtc->id, active);
        fprintf(stream, "diag_dtc_confirmed{dtc_id=\"0x%06lx\"} %u\n", (unsigned long)dtc->id,
                confirmed);
        fprintf(stream, "diag_dtc_occurrences{dtc_id=\"0x%06lx\"} %lu\n", (unsigned long)dtc->id,
                (unsigned long)dtc->occurrence_count);
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
    enum diag_result result = DIAG_OK;
    size_t           i = 0u;

    if (stream == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    result = validate_snapshot(snapshot);
    if (result != DIAG_OK)
    {
        return result;
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
