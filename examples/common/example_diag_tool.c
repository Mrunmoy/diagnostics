#include "example_diag_tool.h"

#include <stdbool.h>
#include <stdio.h>

static const char *example_diag_result_name(enum diag_result result)
{
    switch (result)
    {
        case DIAG_OK:
            return "DIAG_OK";
        case DIAG_ERROR_INVALID_ARGUMENT:
            return "DIAG_ERROR_INVALID_ARGUMENT";
        case DIAG_ERROR_NOT_INITIALIZED:
            return "DIAG_ERROR_NOT_INITIALIZED";
        case DIAG_ERROR_NOT_FOUND:
            return "DIAG_ERROR_NOT_FOUND";
        case DIAG_ERROR_ALREADY_EXISTS:
            return "DIAG_ERROR_ALREADY_EXISTS";
        case DIAG_ERROR_CAPACITY:
            return "DIAG_ERROR_CAPACITY";
        case DIAG_ERROR_STORAGE:
            return "DIAG_ERROR_STORAGE";
        case DIAG_ERROR_TRANSPORT:
            return "DIAG_ERROR_TRANSPORT";
        case DIAG_ERROR_CORRUPT_DATA:
            return "DIAG_ERROR_CORRUPT_DATA";
        case DIAG_ERROR_NOT_SUPPORTED:
            return "DIAG_ERROR_NOT_SUPPORTED";
        default:
            return "DIAG_ERROR_UNKNOWN";
    }
}

void example_diag_write_u32_le(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0xFFu);
    buffer[1] = (uint8_t)((value >> 8u) & 0xFFu);
    buffer[2] = (uint8_t)((value >> 16u) & 0xFFu);
    buffer[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

uint32_t example_diag_read_u32_le(const uint8_t *buffer)
{
    return (uint32_t)((uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8u) |
                      ((uint32_t)buffer[2] << 16u) | ((uint32_t)buffer[3] << 24u));
}

#if DIAG_FEATURE_IDENTITY
static enum diag_result example_diag_append_identity(const struct example_diag_device *device,
                                                     struct example_diag_frame        *response)
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
#endif

#if DIAG_FEATURE_DTC
static enum diag_result example_diag_append_dtc_list(const struct example_diag_device *device,
                                                     struct example_diag_frame        *response)
{
    struct diag_dtc_snapshot dtcs[8] = {0};
    size_t                   count = 0u;
    size_t                   i = 0u;
    enum diag_result         result =
        diag_dtc_list(device->ctx, dtcs, sizeof(dtcs) / sizeof(dtcs[0]), &count);

    if (result != DIAG_OK)
    {
        return result;
    }

    if (count > UINT8_MAX || (2u + (count * EXAMPLE_DIAG_DTC_WIRE_SIZE)) > sizeof(response->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    response->bytes[0] = DIAG_OK;
    response->bytes[1] = (uint8_t)count;
    response->size = 2u + (count * EXAMPLE_DIAG_DTC_WIRE_SIZE);

    for (i = 0u; i < count; ++i)
    {
        uint8_t *record = &response->bytes[2u + (i * EXAMPLE_DIAG_DTC_WIRE_SIZE)];
        const struct diag_dtc_snapshot *dtc = &dtcs[i];

        example_diag_write_u32_le(&record[0], dtc->id);
        record[4] = dtc->status;
        record[5] = (uint8_t)dtc->severity;
        example_diag_write_u32_le(&record[6], dtc->occurrence_count);
    }

    return DIAG_OK;
}

static enum diag_result example_diag_clear_dtc(const struct example_diag_device *device,
                                               const struct example_diag_frame  *request,
                                               struct example_diag_frame        *response)
{
    diag_dtc_id_t    dtc_id = 0u;
    enum diag_result result = DIAG_OK;

    if (request->size != 5u)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    dtc_id = example_diag_read_u32_le(&request->bytes[1]);
    result = diag_dtc_clear(device->ctx, dtc_id);
#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
    if (result == DIAG_OK && device->persisted_size != NULL)
    {
        result = diag_save(device->ctx);
    }
#endif

    response->bytes[0] = (uint8_t)result;
    response->size = 1u;

    return result;
}
#endif

enum diag_result example_diag_device_handle_request(const struct example_diag_device *device,
                                                    const struct example_diag_frame  *request,
                                                    struct example_diag_frame        *response)
{
    enum diag_result result = DIAG_OK;

    if (device == NULL || device->ctx == NULL || request == NULL || response == NULL ||
        request->size == 0u)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    response->size = 0u;

    switch ((enum example_diag_service)request->bytes[0])
    {
        case EXAMPLE_DIAG_SERVICE_READ_IDENTITY:
#if DIAG_FEATURE_IDENTITY
            result = example_diag_append_identity(device, response);
#else
            result = DIAG_ERROR_NOT_SUPPORTED;
#endif
            break;

        case EXAMPLE_DIAG_SERVICE_LIST_DTCS:
#if DIAG_FEATURE_DTC
            result = example_diag_append_dtc_list(device, response);
#else
            result = DIAG_ERROR_NOT_SUPPORTED;
#endif
            break;

        case EXAMPLE_DIAG_SERVICE_CLEAR_DTC:
#if DIAG_FEATURE_DTC
            result = example_diag_clear_dtc(device, request, response);
#else
            result = DIAG_ERROR_NOT_SUPPORTED;
#endif
            break;

        default:
            result = DIAG_ERROR_NOT_SUPPORTED;
            break;
    }

    if (result != DIAG_OK && response->size == 0u)
    {
        response->bytes[0] = (uint8_t)result;
        response->size = 1u;
    }

    return result;
}

static enum diag_result example_diag_tool_exchange(const struct example_diag_device *device,
                                                   const struct example_diag_frame  *request,
                                                   struct example_diag_frame        *response)
{
    enum diag_result result = example_diag_device_handle_request(device, request, response);

    if (result != DIAG_OK)
    {
        fprintf(stderr, "%s tool: request 0x%02x failed: %s\n", device->name, request->bytes[0],
                example_diag_result_name(result));
    }

    return result;
}

static enum diag_result example_diag_tool_read_identity(const struct example_diag_device *device)
{
    struct example_diag_frame request = {{0}, 1u};
    struct example_diag_frame response = {{0}, 0u};
    uint16_t                  ecosystem_id = 0u;
    uint16_t                  product_id = 0u;
    uint16_t                  device_type = 0u;
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_READ_IDENTITY;
    result = example_diag_tool_exchange(device, &request, &response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size != 11u || response.bytes[0] != DIAG_OK)
    {
        fprintf(stderr, "%s tool: malformed identity response\n", device->name);
        return DIAG_ERROR_CORRUPT_DATA;
    }

    ecosystem_id = (uint16_t)((uint16_t)response.bytes[1] | ((uint16_t)response.bytes[2] << 8u));
    product_id = (uint16_t)((uint16_t)response.bytes[3] | ((uint16_t)response.bytes[4] << 8u));
    device_type = (uint16_t)((uint16_t)response.bytes[5] | ((uint16_t)response.bytes[6] << 8u));

    printf("%s tool: identity ecosystem=%u product=%u type=%u instance=%u stage=%u component=%u\n",
           device->name, (unsigned int)ecosystem_id, (unsigned int)product_id,
           (unsigned int)device_type, (unsigned int)response.bytes[7],
           (unsigned int)response.bytes[8], (unsigned int)response.bytes[9]);

    return DIAG_OK;
}

static enum diag_result example_diag_tool_list_dtcs(const struct example_diag_device *device,
                                                    diag_dtc_id_t                     clear_dtc_id,
                                                    uint8_t *out_clear_status)
{
    struct example_diag_frame request = {{0}, 1u};
    struct example_diag_frame response = {{0}, 0u};
    size_t                    count = 0u;
    size_t                    i = 0u;
    bool                      clear_target_found = false;
    enum diag_result          result = DIAG_OK;

    if (out_clear_status == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_LIST_DTCS;
    result = example_diag_tool_exchange(device, &request, &response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size < 2u || response.bytes[0] != DIAG_OK)
    {
        fprintf(stderr, "%s tool: malformed DTC list response\n", device->name);
        return DIAG_ERROR_CORRUPT_DATA;
    }

    count = response.bytes[1];
    if (response.size != (2u + (count * EXAMPLE_DIAG_DTC_WIRE_SIZE)))
    {
        fprintf(stderr, "%s tool: DTC list length mismatch\n", device->name);
        return DIAG_ERROR_CORRUPT_DATA;
    }

    printf("%s tool: DTC count=%lu\n", device->name, (unsigned long)count);
    for (i = 0u; i < count; ++i)
    {
        const uint8_t *record = &response.bytes[2u + (i * EXAMPLE_DIAG_DTC_WIRE_SIZE)];
        const uint32_t dtc_id = example_diag_read_u32_le(&record[0]);
        const uint8_t  status = record[4];
        const uint8_t  severity = record[5];
        const uint32_t occurrence_count = example_diag_read_u32_le(&record[6]);

        printf("%s tool: DTC 0x%06lx status=0x%02x severity=%u occurrences=%lu\n", device->name,
               (unsigned long)dtc_id, (unsigned int)status, (unsigned int)severity,
               (unsigned long)occurrence_count);

        if ((diag_dtc_id_t)dtc_id == clear_dtc_id)
        {
            *out_clear_status = status;
            clear_target_found = true;
        }
    }

    if (clear_dtc_id != 0u && !clear_target_found)
    {
        fprintf(stderr, "%s tool: DTC 0x%06lx was not reported by the device\n", device->name,
                (unsigned long)clear_dtc_id);
        return DIAG_ERROR_NOT_FOUND;
    }

    return DIAG_OK;
}

static enum diag_result example_diag_tool_clear_dtc(const struct example_diag_device *device,
                                                    diag_dtc_id_t                     clear_dtc_id)
{
    struct example_diag_frame request = {{0}, 5u};
    struct example_diag_frame response = {{0}, 0u};
    enum diag_result          result = DIAG_OK;

    request.bytes[0] = EXAMPLE_DIAG_SERVICE_CLEAR_DTC;
    example_diag_write_u32_le(&request.bytes[1], clear_dtc_id);

    result = example_diag_tool_exchange(device, &request, &response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size != 1u || response.bytes[0] != DIAG_OK)
    {
        fprintf(stderr, "%s tool: malformed clear response\n", device->name);
        return DIAG_ERROR_CORRUPT_DATA;
    }

    printf("%s tool: cleared DTC 0x%06lx\n", device->name, (unsigned long)clear_dtc_id);

    return DIAG_OK;
}

enum diag_result example_diag_tool_run_cli(const struct example_diag_device *device,
                                           diag_dtc_id_t                     clear_dtc_id)
{
    enum diag_result result = DIAG_OK;
    uint8_t          status_before_clear = 0xFFu;
    uint8_t          status_after_clear = 0xFFu;

    if (device == NULL || device->name == NULL || device->ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    printf("%s tool: opening diagnostic session\n", device->name);

    result = example_diag_tool_read_identity(device);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = example_diag_tool_list_dtcs(device, clear_dtc_id, &status_before_clear);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (clear_dtc_id != 0u)
    {
        result = example_diag_tool_clear_dtc(device, clear_dtc_id);
        if (result != DIAG_OK)
        {
            return result;
        }

        result = example_diag_tool_list_dtcs(device, clear_dtc_id, &status_after_clear);
        if (result != DIAG_OK)
        {
            return result;
        }
    }

    if (device->persisted_size != NULL)
    {
        printf("%s tool: persisted capsule bytes=%lu\n", device->name,
               (unsigned long)device->persisted_size(device->user));
    }

    if (clear_dtc_id != 0u && status_before_clear == 0u)
    {
        fprintf(stderr, "%s tool: expected clear target to have diagnostic status before clear\n",
                device->name);
        return DIAG_ERROR_CORRUPT_DATA;
    }

    if (clear_dtc_id != 0u && status_after_clear != 0u)
    {
        fprintf(stderr, "%s tool: expected clear target status to become zero\n", device->name);
        return DIAG_ERROR_CORRUPT_DATA;
    }

    printf("%s tool: diagnostic session complete\n", device->name);

    return DIAG_OK;
}
