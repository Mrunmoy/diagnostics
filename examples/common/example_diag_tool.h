#ifndef EXAMPLE_DIAG_TOOL_H
#define EXAMPLE_DIAG_TOOL_H

#include "diag/diag.h"

#include <stddef.h>
#include <stdint.h>

#define EXAMPLE_DIAG_MAX_FRAME_SIZE (160u)
#define EXAMPLE_DIAG_DTC_WIRE_SIZE (10u)

enum example_diag_service
{
    EXAMPLE_DIAG_SERVICE_READ_IDENTITY = 0x01u,
    EXAMPLE_DIAG_SERVICE_LIST_DTCS = 0x02u,
    EXAMPLE_DIAG_SERVICE_CLEAR_DTC = 0x03u
};

struct example_diag_frame
{
    uint8_t bytes[EXAMPLE_DIAG_MAX_FRAME_SIZE];
    size_t  size;
};

struct example_diag_device
{
    const char          *name;
    struct diag_context *ctx;
    size_t (*persisted_size)(void *user);
    void *user;
};

void     example_diag_write_u32_le(uint8_t *buffer, uint32_t value);
uint32_t example_diag_read_u32_le(const uint8_t *buffer);

enum diag_result example_diag_device_handle_request(const struct example_diag_device *device,
                                                    const struct example_diag_frame  *request,
                                                    struct example_diag_frame        *response);

enum diag_result example_diag_tool_run_cli(const struct example_diag_device *device,
                                           diag_dtc_id_t clear_dtc_id);

#endif
