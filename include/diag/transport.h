#ifndef DIAG_TRANSPORT_H
#define DIAG_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

typedef struct diag_transport_ops
{
    diag_result_t (*send)(void *user, const uint8_t *buffer, size_t size);
    diag_result_t (*receive)(void *user, uint8_t *buffer, size_t buffer_size, size_t *bytes_read);
} diag_transport_ops_t;

typedef struct diag_transport
{
    const diag_transport_ops_t *ops;
    void *user;
} diag_transport_t;

#endif
