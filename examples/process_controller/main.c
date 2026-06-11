#include "diag/diag.h"

#include <stdio.h>
#include <string.h>

struct process_store
{
    uint8_t bytes[256];
    size_t  used;
};

static enum diag_result process_load(void *user, uint8_t *buffer, size_t buffer_size,
                                     size_t *bytes_read)
{
    struct process_store *store = (struct process_store *)user;

    if (buffer_size < store->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, store->bytes, store->used);
    *bytes_read = store->used;

    return DIAG_OK;
}

static enum diag_result process_save(void *user, const uint8_t *buffer, size_t size)
{
    struct process_store *store = (struct process_store *)user;

    if (size > sizeof(store->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(store->bytes, buffer, size);
    store->used = size;

    return DIAG_OK;
}

static enum diag_result process_clear(void *user)
{
    struct process_store *store = (struct process_store *)user;

    store->used = 0u;

    return DIAG_OK;
}

int main(void)
{
    static const struct diag_storage_ops storage_ops = {
        .load = process_load,
        .save = process_save,
        .clear = process_clear,
    };

    struct process_store         store = {{0}, 0u};
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    struct diag_dtc_snapshot     dtc_records[3] = {0};
    uint8_t                      capsule_buffer[256] = {0};
    uint32_t                     dirty_flags = DIAG_DIRTY_NONE;
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = dtc_records,
        .capacity = 3u,
        .confirmation_threshold = 2u,
        .aging_threshold = 10u,
    };
    const struct diag_lifecycle_config lifecycle_config = {
        .reset_counter_policy = DIAG_RESET_COUNTER_POLICY_EVERY_N,
        .reset_count_interval = 16u,
        .platform_reset_count = 0u,
    };
    const struct diag_storage storage_adapter = {
        .ops = &storage_ops,
        .user = &store,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 4u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
        .capsule_buffer = capsule_buffer,
        .capsule_buffer_size = sizeof(capsule_buffer),
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK ||
        diag_storage_attach(ctx, &storage_adapter) != DIAG_OK ||
        diag_dtc_attach(ctx, &dtc_config) != DIAG_OK ||
        diag_lifecycle_attach(ctx, &lifecycle_config) != DIAG_OK)
    {
        fprintf(stderr, "process_controller: setup failed\n");
        return 1;
    }

    if (diag_dtc_register_fault(ctx, 11u, 0x030001u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK ||
        diag_dtc_set_fault_test_failed(ctx, 11u) != DIAG_OK ||
        diag_dtc_operation_cycle(ctx) != DIAG_OK ||
        diag_dtc_set_fault_test_failed(ctx, 11u) != DIAG_OK ||
        diag_dtc_operation_cycle(ctx) != DIAG_OK ||
        diag_get_dirty_flags(ctx, &dirty_flags) != DIAG_OK)
    {
        fprintf(stderr, "process_controller: failed to update process DTC\n");
        return 1;
    }

    if ((dirty_flags & DIAG_DIRTY_DTC) == 0u || diag_save(ctx) != DIAG_OK)
    {
        fprintf(stderr, "process_controller: expected dirty DTC and successful save\n");
        return 1;
    }

    printf("process_controller: persisted confirmed DTC using %lu bytes\n",
           (unsigned long)store.used);

    return diag_deinit(ctx) == DIAG_OK ? 0 : 1;
}
