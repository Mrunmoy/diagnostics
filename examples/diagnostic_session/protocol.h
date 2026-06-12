#ifndef DIAGNOSTIC_SESSION_PROTOCOL_H
#define DIAGNOSTIC_SESSION_PROTOCOL_H

#include "diag/diag.h"

#include <stddef.h>
#include <stdint.h>

#define SESSION_MAX_FRAME_SIZE (128u)
#define SESSION_DTC_WIRE_SIZE (10u)

enum session_service
{
    SESSION_SERVICE_READ_IDENTITY = 0x01u,
    SESSION_SERVICE_LIST_DTCS = 0x02u,
    SESSION_SERVICE_CLEAR_DTC = 0x03u
};

struct session_frame
{
    uint8_t bytes[SESSION_MAX_FRAME_SIZE];
    size_t  size;
};

struct diagnostic_device;

enum diag_result diagnostic_device_init(struct diagnostic_device *device);
enum diag_result diagnostic_device_deinit(struct diagnostic_device *device);
enum diag_result diagnostic_device_handle_request(struct diagnostic_device *device,
                                                  const struct session_frame *request,
                                                  struct session_frame       *response);
size_t           diagnostic_device_persisted_size(const struct diagnostic_device *device);

enum diag_result diagnostic_tester_run(struct diagnostic_device *device);

uint32_t session_read_u32_le(const uint8_t *buffer);
void     session_write_u32_le(uint8_t *buffer, uint32_t value);

#endif
