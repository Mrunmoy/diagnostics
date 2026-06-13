#include "device.h"

#include <string.h>

// clang-format off
static enum diag_result viewer_load(
    void *user,
    uint8_t *buffer,
    size_t buffer_size,
    size_t *bytes_read)
// clang-format on
{
    struct diagnostic_viewer_store *store = (struct diagnostic_viewer_store *)user;

    if (buffer_size < store->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, store->bytes, store->used);
    *bytes_read = store->used;

    return DIAG_OK;
}

static enum diag_result viewer_save(void *user, const uint8_t *buffer, size_t size)
{
    struct diagnostic_viewer_store *store = (struct diagnostic_viewer_store *)user;

    if (size > sizeof(store->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(store->bytes, buffer, size);
    store->used = size;

    return DIAG_OK;
}

static enum diag_result viewer_clear_store(void *user)
{
    struct diagnostic_viewer_store *store = (struct diagnostic_viewer_store *)user;

    store->used = 0u;

    return DIAG_OK;
}

static struct diag_storage make_storage(struct diagnostic_viewer_device *device)
{
    static const struct diag_storage_ops ops = {
        .load = viewer_load,
        .save = viewer_save,
        .clear = viewer_clear_store,
    };
    const struct diag_storage storage = {
        .ops = &ops,
        .user = &device->persistent_store,
        .capabilities =
            {
                .erase_value = 0xFFu,
                .write_alignment = 8u,
                .atomic_commit = DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
                .wear_leveling = DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
            },
        .capsule_buffer = device->capsule_buffer,
        .capsule_buffer_size = sizeof(device->capsule_buffer),
    };

    return storage;
}

enum diag_result diagnostic_viewer_device_init(struct diagnostic_viewer_device *device)
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
        .aging_threshold = 6u,
    };
    const struct diag_identity identity = {
        .ecosystem_id = 7u,
        .product_id = 120u,
        .device_type = 8u,
        .device_instance = 2u,
        .firmware_stage = 1u,
        .firmware_component = 5u,
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

    if (diag_dtc_register_fault(device->ctx, 31u, 0x050101u, DIAG_DTC_SEVERITY_ERROR) != DIAG_OK ||
        diag_dtc_register_fault(device->ctx, 32u, 0x050102u, DIAG_DTC_SEVERITY_WARNING) !=
            DIAG_OK ||
        diag_dtc_register_fault(device->ctx, 33u, 0x050103u, DIAG_DTC_SEVERITY_INFO) != DIAG_OK ||
        diag_dtc_set_fault_test_failed(device->ctx, 31u) != DIAG_OK ||
        diag_dtc_operation_cycle(device->ctx) != DIAG_OK || diag_save(device->ctx) != DIAG_OK)
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return DIAG_OK;
}

enum diag_result diagnostic_viewer_device_apply_scenario(struct diagnostic_viewer_device *device,
                                                         uint32_t                         step)
{
    enum diag_result result = DIAG_OK;
    uint32_t         phase = 0u;

    if (device == NULL || device->ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    phase = step % 10u;

    if (phase >= 2u && phase <= 6u)
    {
        result = diag_dtc_set_fault_test_failed(device->ctx, 32u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 3u || phase == 4u)
    {
        result = diag_dtc_set_fault_test_failed(device->ctx, 33u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 4u || phase == 5u || phase == 6u)
    {
        result = diag_dtc_set_fault_test_passed(device->ctx, 31u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    result = diag_dtc_operation_cycle(device->ctx);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (phase == 5u)
    {
        result = diag_dtc_set_fault_test_passed(device->ctx, 33u);
        if (result != DIAG_OK)
        {
            return result;
        }
        result = diag_dtc_clear(device->ctx, 0x050103u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 7u)
    {
        result = diag_dtc_set_fault_test_passed(device->ctx, 32u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 8u)
    {
        result = diag_dtc_clear(device->ctx, 0x050102u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (phase == 9u)
    {
        result = diag_dtc_clear(device->ctx, 0x050101u);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    return diag_save(device->ctx);
}

enum diag_result diagnostic_viewer_device_clear(struct diagnostic_viewer_device *device,
                                                diag_dtc_id_t                    dtc_id)
{
    enum diag_result result = DIAG_OK;

    if (device == NULL || device->ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    result = diag_dtc_clear(device->ctx, dtc_id);
    if (result != DIAG_OK)
    {
        return result;
    }

    return diag_save(device->ctx);
}

enum diag_result diagnostic_viewer_device_deinit(struct diagnostic_viewer_device *device)
{
    if (device == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    return diag_deinit(device->ctx);
}

size_t diagnostic_viewer_device_persisted_size(void *user)
{
    const struct diagnostic_viewer_device *device = (const struct diagnostic_viewer_device *)user;

    if (device == NULL)
    {
        return 0u;
    }

    return device->persistent_store.used;
}
