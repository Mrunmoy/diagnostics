#include "device.h"

#include <string.h>

// clang-format off
static enum diag_result grafana_reader_load(
    void *user,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *bytes_read)
// clang-format on
{
    struct grafana_reader_store *store = (struct grafana_reader_store *)user;

    if (buffer_size < store->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, store->bytes, store->used);
    *bytes_read = store->used;

    return DIAG_OK;
}

static enum diag_result grafana_reader_save(void *user, const uint8_t *buffer, size_t size)
{
    struct grafana_reader_store *store = (struct grafana_reader_store *)user;

    if (size > sizeof(store->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(store->bytes, buffer, size);
    store->used = size;

    return DIAG_OK;
}

static enum diag_result grafana_reader_clear(void *user)
{
    struct grafana_reader_store *store = (struct grafana_reader_store *)user;

    store->used = 0u;

    return DIAG_OK;
}

static struct diag_storage make_storage(struct grafana_reader_device *device)
{
    static const struct diag_storage_ops ops = {
        .load = grafana_reader_load,
        .save = grafana_reader_save,
        .clear = grafana_reader_clear,
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

enum diag_result grafana_reader_device_init(struct grafana_reader_device *device)
{
    if (device == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    const struct diag_config     config = {0};
    const struct diag_dtc_config dtc_config = {
        .records = device->dtc_records,
        .capacity = sizeof(device->dtc_records) / sizeof(device->dtc_records[0]),
        .confirmation_threshold = 1u,
        .aging_threshold = 10u,
    };
    const struct diag_identity identity = {
        .ecosystem_id = 7u,
        .product_id = 90u,
        .device_type = 4u,
        .device_instance = 3u,
        .firmware_stage = 1u,
        .firmware_component = 1u,
        .reserved = 0u,
    };
    struct diag_storage storage = {0};

    memset(device, 0, sizeof(*device));
    storage = make_storage(device);

    if (diag_init(&device->context_storage, &config, &device->ctx) != DIAG_OK ||
        diag_identity_attach(device->ctx, &identity) != DIAG_OK ||
        diag_storage_attach(device->ctx, &storage) != DIAG_OK ||
        diag_dtc_attach(device->ctx, &dtc_config) != DIAG_OK)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    if (diag_dtc_register_fault(device->ctx, 21u, 0x040101u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK ||
        diag_dtc_register_fault(device->ctx, 22u, 0x040102u, DIAG_DTC_SEVERITY_WARNING) !=
            DIAG_OK ||
        diag_dtc_set_fault_test_failed(device->ctx, 21u) != DIAG_OK ||
        diag_dtc_operation_cycle(device->ctx) != DIAG_OK || diag_save(device->ctx) != DIAG_OK)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return DIAG_OK;
}

enum diag_result grafana_reader_device_apply_scenario(struct grafana_reader_device *device,
                                                      uint32_t                      step)
{
    enum diag_result result = DIAG_OK;
    uint32_t         phase = 0u;

    if (device == NULL || device->ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    phase = step % 8u;

    if (phase >= 2u && phase <= 4u)
    {
        result = diag_dtc_set_fault_test_failed(device->ctx, 22u);
        if (result != DIAG_OK)
        {
            return result;
        }
        result = diag_dtc_operation_cycle(device->ctx);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 4u || phase == 5u)
    {
        result = diag_dtc_set_fault_test_passed(device->ctx, 21u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 6u)
    {
        result = diag_dtc_set_fault_test_passed(device->ctx, 21u);
        if (result != DIAG_OK)
        {
            return result;
        }
        result = diag_dtc_set_fault_test_failed(device->ctx, 21u);
        if (result != DIAG_OK)
        {
            return result;
        }
        result = diag_dtc_operation_cycle(device->ctx);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 7u)
    {
        result = diag_dtc_clear(device->ctx, 0x040101u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    return diag_save(device->ctx);
}

enum diag_result grafana_reader_device_deinit(struct grafana_reader_device *device)
{
    if (device == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    return diag_deinit(device->ctx);
}

size_t grafana_reader_device_persisted_size(void *user)
{
    const struct grafana_reader_device *device = (const struct grafana_reader_device *)user;

    if (device == NULL)
    {
        return 0u;
    }

    return device->persistent_store.used;
}
