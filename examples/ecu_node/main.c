#include "diag/diag.h"
#include "example_diag_tool.h"

#include <stdio.h>
#include <string.h>

struct ecu_store
{
    uint8_t bytes[384];
    size_t  used;
};

struct ecu_transport
{
    uint8_t tx[32];
    size_t  tx_size;
};

static size_t ecu_persisted_size(void *user)
{
    const struct ecu_store *store = (const struct ecu_store *)user;

    return store->used;
}

static enum diag_result ecu_load(void *user, uint8_t *buffer, size_t buffer_size,
                                 size_t *bytes_read)
{
    struct ecu_store *store = (struct ecu_store *)user;

    if (buffer_size < store->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, store->bytes, store->used);
    *bytes_read = store->used;

    return DIAG_OK;
}

static enum diag_result ecu_save(void *user, const uint8_t *buffer, size_t size)
{
    struct ecu_store *store = (struct ecu_store *)user;

    if (size > sizeof(store->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(store->bytes, buffer, size);
    store->used = size;

    return DIAG_OK;
}

static enum diag_result ecu_clear(void *user)
{
    struct ecu_store *store = (struct ecu_store *)user;

    store->used = 0u;

    return DIAG_OK;
}

static enum diag_result ecu_send(void *user, const uint8_t *buffer, size_t size)
{
    struct ecu_transport *transport = (struct ecu_transport *)user;

    if (size > sizeof(transport->tx))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(transport->tx, buffer, size);
    transport->tx_size = size;

    return DIAG_OK;
}

static enum diag_result ecu_receive(void *user, uint8_t *buffer, size_t buffer_size,
                                    size_t *bytes_read)
{
    struct ecu_transport *transport = (struct ecu_transport *)user;

    if (buffer_size < transport->tx_size)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, transport->tx, transport->tx_size);
    *bytes_read = transport->tx_size;

    return DIAG_OK;
}

int main(void)
{
    static const struct diag_storage_ops storage_ops = {
        .load = ecu_load,
        .save = ecu_save,
        .clear = ecu_clear,
    };
    static const struct diag_transport_ops transport_ops = {
        .send = ecu_send,
        .receive = ecu_receive,
    };

    struct ecu_store             store = {{0}, 0u};
    struct ecu_transport         bus = {{0}, 0u};
    struct diag_context_storage  storage = {0};
    struct diag_context         *ctx = NULL;
    struct diag_dtc_snapshot     dtc_records[6] = {0};
    struct diag_dtc_snapshot     fault = {0};
    uint8_t                      capsule_buffer[384] = {0};
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = dtc_records,
        .capacity = 6u,
        .confirmation_threshold = 1u,
        .aging_threshold = 40u,
    };
    const struct diag_lifecycle_config lifecycle_config = {
        .reset_counter_policy = DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY,
        .reset_count_interval = 0u,
        .platform_reset_count = 0u,
    };
    const struct diag_identity identity = {
        .ecosystem_id = 1u,
        .product_id = 50u,
        .device_type = 5u,
        .device_instance = 2u,
        .firmware_stage = 1u,
        .firmware_component = 1u,
        .reserved = 0u,
    };
    const struct diag_storage storage_adapter = {
        .ops = &storage_ops,
        .user = &store,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 8u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
        .capsule_buffer = capsule_buffer,
        .capsule_buffer_size = sizeof(capsule_buffer),
    };
    const struct diag_transport transport = {
        .ops = &transport_ops,
        .user = &bus,
    };

    if (diag_init(&storage, &config, &ctx) != DIAG_OK ||
        diag_identity_attach(ctx, &identity) != DIAG_OK ||
        diag_storage_attach(ctx, &storage_adapter) != DIAG_OK ||
        diag_transport_attach(ctx, &transport) != DIAG_OK ||
        diag_dtc_attach(ctx, &dtc_config) != DIAG_OK ||
        diag_lifecycle_attach(ctx, &lifecycle_config) != DIAG_OK)
    {
        fprintf(stderr, "ecu_node: setup failed\n");
        return 1;
    }

    if (diag_dtc_register_fault(ctx, 77u, 0x050001u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK ||
        diag_dtc_set_fault_test_failed(ctx, 77u) != DIAG_OK ||
        diag_dtc_operation_cycle(ctx) != DIAG_OK ||
        diag_dtc_get(ctx, 0x050001u, &fault) != DIAG_OK || diag_save(ctx) != DIAG_OK)
    {
        fprintf(stderr, "ecu_node: diagnostic flow failed\n");
        return 1;
    }

    if ((fault.status & DIAG_DTC_STATUS_CONFIRMED) == 0u)
    {
        fprintf(stderr, "ecu_node: expected confirmed DTC\n");
        return 1;
    }

    printf("ecu_node: confirmed DTC 0x%06lx, saved %lu bytes\n", (unsigned long)fault.id,
           (unsigned long)store.used);

    {
        const struct example_diag_device device = {
            .name = "ecu_node",
            .ctx = ctx,
            .persisted_size = ecu_persisted_size,
            .user = &store,
        };

        if (example_diag_tool_run_cli(&device, 0x050001u) != DIAG_OK)
        {
            fprintf(stderr, "ecu_node: diagnostic tool flow failed\n");
            return 1;
        }
    }

    return diag_deinit(ctx) == DIAG_OK ? 0 : 1;
}
