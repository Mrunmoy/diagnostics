#include "device.h"
#include "example_diag_tool.h"

#include "diag/diag.h"

#include <stdbool.h>
#include <stdio.h>

struct catalog_identity_entry
{
    uint16_t    ecosystem_id;
    uint16_t    product_id;
    uint16_t    device_type;
    uint8_t     firmware_stage;
    uint8_t     firmware_component;
    const char *catalog_version;
    const char *product_name;
    const char *device_role;
};

struct catalog_dtc_entry
{
    uint16_t      ecosystem_id;
    uint16_t      product_id;
    uint16_t      device_type;
    uint8_t       firmware_stage;
    uint8_t       firmware_component;
    diag_dtc_id_t dtc_id;
    const char   *name;
    const char   *meaning;
    const char   *service_action;
};

static const struct catalog_identity_entry catalog_identities[] = {
    {
        .ecosystem_id = 7u,
        .product_id = 42u,
        .device_type = 3u,
        .firmware_stage = 1u,
        .firmware_component = 2u,
        .catalog_version = "example-catalog-2026.06",
        .product_name = "Reference Control Node",
        .device_role = "bench diagnostic target",
    },
};

static const struct catalog_dtc_entry catalog_dtcs[] = {
    {
        .ecosystem_id = 7u,
        .product_id = 42u,
        .device_type = 3u,
        .firmware_stage = 1u,
        .firmware_component = 2u,
        .dtc_id = 0x030101u,
        .name = "Supply brownout detected",
        .meaning =
            "The device observed a failed supply monitor during the current operation cycle.",
        .service_action =
            "Check input supply range, connector retention, and brownout reset history.",
    },
    {
        .ecosystem_id = 7u,
        .product_id = 42u,
        .device_type = 3u,
        .firmware_stage = 1u,
        .firmware_component = 2u,
        .dtc_id = 0x030102u,
        .name = "Configuration fallback active",
        .meaning =
            "The device is running with a safe default configuration after validation failed.",
        .service_action =
            "Read configuration source, verify checksum, and reload the product profile.",
    },
};

static bool status_mask_is_set(uint8_t status, uint8_t mask)
{
    return (status & mask) != 0u;
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
        case DIAG_DTC_SEVERITY_CRITICAL:
            return "critical";
        default:
            return "unknown";
    }
}

static const struct catalog_identity_entry *
find_identity_entry(const struct example_diag_identity_snapshot *identity)
{
    size_t i = 0u;

    for (i = 0u; i < (sizeof(catalog_identities) / sizeof(catalog_identities[0])); ++i)
    {
        const struct catalog_identity_entry *entry = &catalog_identities[i];

        if (entry->ecosystem_id == identity->ecosystem_id &&
            entry->product_id == identity->product_id &&
            entry->device_type == identity->device_type &&
            entry->firmware_stage == identity->firmware_stage &&
            entry->firmware_component == identity->firmware_component)
        {
            return entry;
        }
    }

    return NULL;
}

static const struct catalog_dtc_entry *
find_dtc_entry(const struct example_diag_identity_snapshot *identity, diag_dtc_id_t dtc_id)
{
    size_t i = 0u;

    for (i = 0u; i < (sizeof(catalog_dtcs) / sizeof(catalog_dtcs[0])); ++i)
    {
        const struct catalog_dtc_entry *entry = &catalog_dtcs[i];

        if (entry->ecosystem_id == identity->ecosystem_id &&
            entry->product_id == identity->product_id &&
            entry->device_type == identity->device_type &&
            entry->firmware_stage == identity->firmware_stage &&
            entry->firmware_component == identity->firmware_component && entry->dtc_id == dtc_id)
        {
            return entry;
        }
    }

    return NULL;
}

static void print_status_summary(uint8_t status)
{
    printf("status=0x%02x active=%s pending=%s confirmed=%s failed_since_clear=%s\n",
           (unsigned int)status,
           status_mask_is_set(status, DIAG_DTC_STATUS_TEST_FAILED) ? "yes" : "no",
           status_mask_is_set(status, DIAG_DTC_STATUS_PENDING) ? "yes" : "no",
           status_mask_is_set(status, DIAG_DTC_STATUS_CONFIRMED) ? "yes" : "no",
           status_mask_is_set(status, DIAG_DTC_STATUS_TEST_FAILED_SINCE_CLEAR) ? "yes" : "no");
}

static enum diag_result print_catalog_view(const struct example_diag_tool_snapshot *snapshot)
{
    const struct catalog_identity_entry *identity_entry = NULL;
    size_t                               i = 0u;
    size_t                               described_count = 0u;

    if (snapshot == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    identity_entry = find_identity_entry(&snapshot->identity);
    if (identity_entry == NULL)
    {
        fprintf(stderr, "catalog_reader: no catalog for identity ecosystem=%u product=%u type=%u\n",
                (unsigned int)snapshot->identity.ecosystem_id,
                (unsigned int)snapshot->identity.product_id,
                (unsigned int)snapshot->identity.device_type);
        return DIAG_ERROR_NOT_FOUND;
    }

    printf("catalog_reader: catalog=%s\n", identity_entry->catalog_version);
    printf("catalog_reader: product=%s role=%s instance=%u\n", identity_entry->product_name,
           identity_entry->device_role, (unsigned int)snapshot->identity.device_instance);
    printf(
        "catalog_reader: compact identity ecosystem=%u product=%u type=%u stage=%u component=%u\n",
        (unsigned int)snapshot->identity.ecosystem_id, (unsigned int)snapshot->identity.product_id,
        (unsigned int)snapshot->identity.device_type,
        (unsigned int)snapshot->identity.firmware_stage,
        (unsigned int)snapshot->identity.firmware_component);
    printf("catalog_reader: DTC count=%lu persisted_capsule_bytes=%lu\n",
           (unsigned long)snapshot->dtc_count, (unsigned long)snapshot->persisted_size);

    for (i = 0u; i < snapshot->dtc_count; ++i)
    {
        const struct example_diag_dtc_snapshot *dtc = &snapshot->dtcs[i];
        const struct catalog_dtc_entry *entry = find_dtc_entry(&snapshot->identity, dtc->id);

        printf("catalog_reader: DTC 0x%06lx severity=%s occurrences=%lu ", (unsigned long)dtc->id,
               severity_name(dtc->severity), (unsigned long)dtc->occurrence_count);
        print_status_summary(dtc->status);

        if (entry == NULL)
        {
            printf("  name: unknown catalog entry\n");
            continue;
        }

        ++described_count;
        printf("  name: %s\n", entry->name);
        printf("  meaning: %s\n", entry->meaning);
        printf("  action: %s\n", entry->service_action);
    }

    if (described_count != snapshot->dtc_count)
    {
        fprintf(stderr, "catalog_reader: one or more DTCs did not have catalog entries\n");
        return DIAG_ERROR_NOT_FOUND;
    }

    return DIAG_OK;
}

int main(void)
{
    struct diagnostic_device          device = {0};
    struct example_diag_tool_snapshot snapshot = {0};
    const struct example_diag_device  endpoint = {
         .name = "catalog_reader",
         .ctx = NULL,
         .persisted_size = diagnostic_device_persisted_size,
         .user = &device,
    };
    struct example_diag_device endpoint_with_context = endpoint;
    enum diag_result           result = DIAG_OK;

    result = diagnostic_device_init(&device);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "catalog_reader: failed to initialize diagnostic device\n");
        return 1;
    }

    endpoint_with_context.ctx = device.ctx;

    result = example_diag_tool_collect_snapshot(&endpoint_with_context, &snapshot);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "catalog_reader: failed to collect diagnostic snapshot\n");
        return 1;
    }

    result = print_catalog_view(&snapshot);
    if (result != DIAG_OK)
    {
        return 1;
    }

    return 0;
}
