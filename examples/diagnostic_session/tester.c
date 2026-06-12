#include "device.h"

#include <stdio.h>

static const char *tester_result_name(enum diag_result result)
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

static enum diag_result tester_exchange(struct diagnostic_device   *device,
                                        const struct session_frame *request,
                                        struct session_frame       *response)
{
    enum diag_result result = diagnostic_device_handle_request(device, request, response);

    if (result != DIAG_OK)
    {
        fprintf(stderr, "tester: request 0x%02x failed: %s\n", request->bytes[0],
                tester_result_name(result));
    }

    return result;
}

static enum diag_result tester_read_identity(struct diagnostic_device *device)
{
    struct session_frame request = {{0}, 1u};
    struct session_frame response = {{0}, 0u};
    uint16_t             ecosystem_id = 0u;
    uint16_t             product_id = 0u;
    uint16_t             device_type = 0u;
    enum diag_result     result = DIAG_OK;

    request.bytes[0] = SESSION_SERVICE_READ_IDENTITY;
    result = tester_exchange(device, &request, &response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size != 11u || response.bytes[0] != DIAG_OK)
    {
        fprintf(stderr, "tester: malformed identity response\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    ecosystem_id = (uint16_t)((uint16_t)response.bytes[1] | ((uint16_t)response.bytes[2] << 8u));
    product_id = (uint16_t)((uint16_t)response.bytes[3] | ((uint16_t)response.bytes[4] << 8u));
    device_type = (uint16_t)((uint16_t)response.bytes[5] | ((uint16_t)response.bytes[6] << 8u));

    printf("tester: identity ecosystem=%u product=%u type=%u instance=%u stage=%u component=%u\n",
           (unsigned int)ecosystem_id, (unsigned int)product_id, (unsigned int)device_type,
           (unsigned int)response.bytes[7], (unsigned int)response.bytes[8],
           (unsigned int)response.bytes[9]);

    return DIAG_OK;
}

static enum diag_result tester_list_dtcs(struct diagnostic_device *device, size_t *out_count,
                                         uint8_t *out_primary_status)
{
    struct session_frame request = {{0}, 1u};
    struct session_frame response = {{0}, 0u};
    size_t               count = 0u;
    size_t               i = 0u;
    enum diag_result     result = DIAG_OK;

    if (out_count == NULL || out_primary_status == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    request.bytes[0] = SESSION_SERVICE_LIST_DTCS;
    result = tester_exchange(device, &request, &response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size < 2u || response.bytes[0] != DIAG_OK)
    {
        fprintf(stderr, "tester: malformed DTC list response\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    count = response.bytes[1];
    if (response.size != (2u + (count * SESSION_DTC_WIRE_SIZE)))
    {
        fprintf(stderr, "tester: DTC list length mismatch\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    printf("tester: DTC count=%lu\n", (unsigned long)count);
    for (i = 0u; i < count; ++i)
    {
        const uint8_t *record = &response.bytes[2u + (i * SESSION_DTC_WIRE_SIZE)];
        const uint32_t dtc_id = session_read_u32_le(&record[0]);
        const uint8_t  status = record[4];
        const uint8_t  severity = record[5];
        const uint32_t occurrence_count = session_read_u32_le(&record[6]);

        printf("tester: DTC 0x%06lx status=0x%02x severity=%u occurrences=%lu\n",
               (unsigned long)dtc_id, (unsigned int)status, (unsigned int)severity,
               (unsigned long)occurrence_count);

        if (dtc_id == 0x030101u)
        {
            *out_primary_status = status;
        }
    }

    *out_count = count;

    return DIAG_OK;
}

static enum diag_result tester_clear_dtc(struct diagnostic_device *device, diag_dtc_id_t dtc_id)
{
    struct session_frame request = {{0}, 5u};
    struct session_frame response = {{0}, 0u};
    enum diag_result     result = DIAG_OK;

    request.bytes[0] = SESSION_SERVICE_CLEAR_DTC;
    session_write_u32_le(&request.bytes[1], dtc_id);

    result = tester_exchange(device, &request, &response);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (response.size != 1u || response.bytes[0] != DIAG_OK)
    {
        fprintf(stderr, "tester: malformed clear response\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    printf("tester: cleared DTC 0x%06lx\n", (unsigned long)dtc_id);

    return DIAG_OK;
}

enum diag_result diagnostic_tester_run(struct diagnostic_device *device)
{
    size_t           dtc_count_before_clear = 0u;
    size_t           dtc_count_after_clear = 0u;
    uint8_t          primary_status_before_clear = 0u;
    uint8_t          primary_status_after_clear = 0xFFu;
    enum diag_result result = DIAG_OK;

    printf("tester: opening diagnostic session\n");

    result = tester_read_identity(device);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = tester_list_dtcs(device, &dtc_count_before_clear, &primary_status_before_clear);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (dtc_count_before_clear != 2u)
    {
        fprintf(stderr, "tester: expected two registered DTCs before clear\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    if ((primary_status_before_clear & DIAG_DTC_STATUS_CONFIRMED) == 0u)
    {
        fprintf(stderr, "tester: expected primary DTC to be confirmed before clear\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    result = tester_clear_dtc(device, 0x030101u);
    if (result != DIAG_OK)
    {
        return result;
    }

    result = tester_list_dtcs(device, &dtc_count_after_clear, &primary_status_after_clear);
    if (result != DIAG_OK)
    {
        return result;
    }

    if (dtc_count_after_clear != 2u)
    {
        fprintf(stderr, "tester: expected registered DTCs to remain after clear\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    if (primary_status_after_clear != 0u)
    {
        fprintf(stderr, "tester: expected primary DTC status to be clear\n");
        return DIAG_ERROR_CORRUPT_DATA;
    }

    printf("tester: persisted capsule bytes=%lu\n",
           (unsigned long)diagnostic_device_persisted_size(device));
    printf("tester: diagnostic session complete\n");

    return DIAG_OK;
}
