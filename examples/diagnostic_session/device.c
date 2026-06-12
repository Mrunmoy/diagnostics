#include "device.h"

#include <string.h>

static enum diag_result session_load(void *user, uint8_t *buffer, size_t buffer_size,
                                     size_t *bytes_read)
{
    struct session_storage *store = (struct session_storage *)user;

    if (buffer_size < store->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, store->bytes, store->used);
    *bytes_read = store->used;

    return DIAG_OK;
}

static enum diag_result session_save(void *user, const uint8_t *buffer, size_t size)
{
    struct session_storage *store = (struct session_storage *)user;

    if (size > sizeof(store->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(store->bytes, buffer, size);
    store->used = size;

    return DIAG_OK;
}

static enum diag_result session_clear(void *user)
{
    struct session_storage *store = (struct session_storage *)user;

    store->used = 0u;

    return DIAG_OK;
}

static struct diag_storage make_storage(struct diagnostic_device *device)
{
    static const struct diag_storage_ops ops = {
        .load = session_load,
        .save = session_save,
        .clear = session_clear,
    };
    const struct diag_storage storage = {
        .ops = &ops,
        .user = &device->persistent_store,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 4u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
        .capsule_buffer = device->capsule_buffer,
        .capsule_buffer_size = sizeof(device->capsule_buffer),
    };

    return storage;
}

enum diag_result diagnostic_device_init(struct diagnostic_device *device)
{
    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = device->dtc_records,
        .capacity = sizeof(device->dtc_records) / sizeof(device->dtc_records[0]),
        .confirmation_threshold = 1u,
        .aging_threshold = 10u,
    };
    const struct diag_identity identity = {
        .ecosystem_id = 7u,
        .product_id = 42u,
        .device_type = 3u,
        .device_instance = 1u,
        .firmware_stage = 1u,
        .firmware_component = 2u,
        .reserved = 0u,
    };
    struct diag_storage storage = {0};

    if (device == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    memset(device, 0, sizeof(*device));
    storage = make_storage(device);

    if (diag_init(&device->context_storage, &config, &device->ctx) != DIAG_OK ||
        diag_identity_attach(device->ctx, &identity) != DIAG_OK ||
        diag_storage_attach(device->ctx, &storage) != DIAG_OK ||
        diag_dtc_attach(device->ctx, &dtc_config) != DIAG_OK)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    if (diag_dtc_register_fault(device->ctx, 11u, 0x030101u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK ||
        diag_dtc_register_fault(device->ctx, 12u, 0x030102u, DIAG_DTC_SEVERITY_WARNING) !=
            DIAG_OK ||
        diag_dtc_set_fault_test_failed(device->ctx, 11u) != DIAG_OK ||
        diag_dtc_operation_cycle(device->ctx) != DIAG_OK || diag_save(device->ctx) != DIAG_OK)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return DIAG_OK;
}

enum diag_result diagnostic_device_deinit(struct diagnostic_device *device)
{
    if (device == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    return diag_deinit(device->ctx);
}

size_t diagnostic_device_persisted_size(void *user)
{
    const struct diagnostic_device *device = (const struct diagnostic_device *)user;

    if (device == NULL)
    {
        return 0u;
    }

    return device->persistent_store.used;
}
