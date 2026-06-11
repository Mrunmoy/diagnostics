#include "diag/diag.h"

#include <stdio.h>
#include <string.h>

struct fake_storage
{
    uint8_t bytes[256];
    size_t  used;
};

static enum diag_result fake_load(void *user, uint8_t *buffer, size_t buffer_size,
                                  size_t *bytes_read)
{
    struct fake_storage *fake = (struct fake_storage *)user;

    if (buffer_size < fake->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, fake->bytes, fake->used);
    *bytes_read = fake->used;

    return DIAG_OK;
}

static enum diag_result fake_save(void *user, const uint8_t *buffer, size_t size)
{
    struct fake_storage *fake = (struct fake_storage *)user;

    if (size > sizeof(fake->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(fake->bytes, buffer, size);
    fake->used = size;

    return DIAG_OK;
}

static enum diag_result fake_clear(void *user)
{
    struct fake_storage *fake = (struct fake_storage *)user;

    fake->used = 0u;

    return DIAG_OK;
}

static struct diag_storage make_storage(struct fake_storage *fake, uint8_t *capsule_buffer,
                                        size_t capsule_buffer_size)
{
    static const struct diag_storage_ops ops = {
        .load = fake_load,
        .save = fake_save,
        .clear = fake_clear,
    };
    const struct diag_storage storage = {
        .ops = &ops,
        .user = fake,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 4u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
        .capsule_buffer = capsule_buffer,
        .capsule_buffer_size = capsule_buffer_size,
    };

    return storage;
}

int main(void)
{
    struct fake_storage          fake = {{0}, 0u};
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    struct diag_dtc_snapshot     dtc_records[4] = {0};
    struct diag_dtc_snapshot     restored_fault = {0};
    uint8_t                      capsule_buffer[256] = {0};
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = dtc_records,
        .capacity = 4u,
    };
    const struct diag_lifecycle_config lifecycle_config = {
        .reset_counter_policy = DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY,
        .reset_count_interval = 0u,
        .platform_reset_count = 0u,
    };
    const struct diag_identity identity = {
        .ecosystem_id = 1u,
        .product_id = 20u,
        .device_type = 2u,
        .device_instance = 1u,
        .firmware_stage = 1u,
        .firmware_component = 1u,
        .reserved = 0u,
    };
    struct diag_storage persistent_storage =
        make_storage(&fake, capsule_buffer, sizeof(capsule_buffer));

    if (diag_init(&storage, &config, &ctx) != DIAG_OK ||
        diag_identity_attach(ctx, &identity) != DIAG_OK ||
        diag_storage_attach(ctx, &persistent_storage) != DIAG_OK ||
        diag_dtc_attach(ctx, &dtc_config) != DIAG_OK ||
        diag_lifecycle_attach(ctx, &lifecycle_config) != DIAG_OK)
    {
        fprintf(stderr, "industrial_oven: setup failed\n");
        return 1;
    }

    if (diag_dtc_register_fault(ctx, 100u, 0x020001u, DIAG_DTC_SEVERITY_CRITICAL) != DIAG_OK ||
        diag_dtc_set_fault_test_failed(ctx, 100u) != DIAG_OK ||
        diag_dtc_operation_cycle(ctx) != DIAG_OK ||
        diag_lifecycle_observe_reset(ctx, DIAG_RESET_REASON_WATCHDOG) != DIAG_OK ||
        diag_save(ctx) != DIAG_OK)
    {
        fprintf(stderr, "industrial_oven: failed to save diagnostics\n");
        return 1;
    }

    memset(dtc_records, 0, sizeof(dtc_records));
    if (diag_load(ctx) != DIAG_OK || diag_dtc_get(ctx, 0x020001u, &restored_fault) != DIAG_OK)
    {
        fprintf(stderr, "industrial_oven: failed to restore diagnostics\n");
        return 1;
    }

    printf("industrial_oven: persisted DTC 0x%06lx in %lu bytes\n",
           (unsigned long)restored_fault.id, (unsigned long)fake.used);

    return diag_deinit(ctx) == DIAG_OK ? 0 : 1;
}
