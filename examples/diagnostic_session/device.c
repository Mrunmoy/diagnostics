#include "device.h"

#include <string.h>

static enum diag_result example_load(void *user, uint8_t *buffer, size_t buffer_size,
                                     size_t *bytes_read)
{
    struct example_storage *store = (struct example_storage *)user;

    if (buffer_size < store->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(buffer, store->bytes, store->used);
    *bytes_read = store->used;

    return DIAG_OK;
}

static enum diag_result example_save(void *user, const uint8_t *buffer, size_t size)
{
    struct example_storage *store = (struct example_storage *)user;

    if (size > sizeof(store->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    memcpy(store->bytes, buffer, size);
    store->used = size;

    return DIAG_OK;
}

static enum diag_result example_clear(void *user)
{
    struct example_storage *store = (struct example_storage *)user;

    store->used = 0u;

    return DIAG_OK;
}

static struct diag_storage make_storage(struct diagnostic_device *device)
{
    static const struct diag_storage_ops ops = {
        .load = example_load,
        .save = example_save,
        .clear = example_clear,
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

static enum diag_result device_append_identity(struct diagnostic_device *device,
                                               struct session_frame     *response)
{
    struct diag_identity identity = {0};
    enum diag_result     result = diag_identity_get(device->ctx, &identity);

    if (result != DIAG_OK)
    {
        return result;
    }

    response->bytes[0] = DIAG_OK;
    response->bytes[1] = (uint8_t)(identity.ecosystem_id & 0xFFu);
    response->bytes[2] = (uint8_t)((identity.ecosystem_id >> 8u) & 0xFFu);
    response->bytes[3] = (uint8_t)(identity.product_id & 0xFFu);
    response->bytes[4] = (uint8_t)((identity.product_id >> 8u) & 0xFFu);
    response->bytes[5] = (uint8_t)(identity.device_type & 0xFFu);
    response->bytes[6] = (uint8_t)((identity.device_type >> 8u) & 0xFFu);
    response->bytes[7] = identity.device_instance;
    response->bytes[8] = identity.firmware_stage;
    response->bytes[9] = identity.firmware_component;
    response->bytes[10] = identity.reserved;
    response->size = 11u;

    return DIAG_OK;
}

static enum diag_result device_append_dtc_list(struct diagnostic_device *device,
                                               struct session_frame     *response)
{
    struct diag_dtc_snapshot dtcs[4] = {0};
    size_t                   count = 0u;
    size_t                   i = 0u;
    enum diag_result         result =
        diag_dtc_list(device->ctx, dtcs, sizeof(dtcs) / sizeof(dtcs[0]), &count);

    if (result != DIAG_OK)
    {
        return result;
    }

    if ((2u + (count * SESSION_DTC_WIRE_SIZE)) > sizeof(response->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    response->bytes[0] = DIAG_OK;
    response->bytes[1] = (uint8_t)count;
    response->size = 2u + (count * SESSION_DTC_WIRE_SIZE);

    for (i = 0u; i < count; ++i)
    {
        uint8_t                        *record = &response->bytes[2u + (i * SESSION_DTC_WIRE_SIZE)];
        const struct diag_dtc_snapshot *dtc = &dtcs[i];

        session_write_u32_le(&record[0], dtc->id);
        record[4] = dtc->status;
        record[5] = (uint8_t)dtc->severity;
        session_write_u32_le(&record[6], dtc->occurrence_count);
    }

    return DIAG_OK;
}

static enum diag_result device_clear_dtc(struct diagnostic_device   *device,
                                         const struct session_frame *request,
                                         struct session_frame       *response)
{
    diag_dtc_id_t    dtc_id = 0u;
    enum diag_result result = DIAG_OK;

    if (request->size != 5u)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    dtc_id = session_read_u32_le(&request->bytes[1]);
    result = diag_dtc_clear(device->ctx, dtc_id);
    if (result == DIAG_OK)
    {
        result = diag_save(device->ctx);
    }

    response->bytes[0] = (uint8_t)result;
    response->size = 1u;

    return result;
}

uint32_t session_read_u32_le(const uint8_t *buffer)
{
    return (uint32_t)((uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8u) |
                      ((uint32_t)buffer[2] << 16u) | ((uint32_t)buffer[3] << 24u));
}

void session_write_u32_le(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFu);
    buffer[1] = (uint8_t)((value >> 8u) & 0xFFu);
    buffer[2] = (uint8_t)((value >> 16u) & 0xFFu);
    buffer[3] = (uint8_t)((value >> 24u) & 0xFFu);
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

enum diag_result diagnostic_device_handle_request(struct diagnostic_device   *device,
                                                  const struct session_frame *request,
                                                  struct session_frame       *response)
{
    enum diag_result result = DIAG_OK;

    if (device == NULL || request == NULL || response == NULL || request->size == 0u)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    response->size = 0u;

    switch ((enum session_service)request->bytes[0])
    {
        case SESSION_SERVICE_READ_IDENTITY:
            result = device_append_identity(device, response);
            break;

        case SESSION_SERVICE_LIST_DTCS:
            result = device_append_dtc_list(device, response);
            break;

        case SESSION_SERVICE_CLEAR_DTC:
            result = device_clear_dtc(device, request, response);
            break;

        default:
            result = DIAG_ERROR_NOT_SUPPORTED;
            response->bytes[0] = (uint8_t)result;
            response->size = 1u;
            break;
    }

    if (result != DIAG_OK && response->size == 0u)
    {
        response->bytes[0] = (uint8_t)result;
        response->size = 1u;
    }

    return result;
}

size_t diagnostic_device_persisted_size(const struct diagnostic_device *device)
{
    if (device == NULL)
    {
        return 0u;
    }

    return device->persistent_store.used;
}
