#ifndef DIAG_TRANSPORT_H
#define DIAG_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

struct diag_transport_ops
{
    enum diag_result (*send)(void *user, const uint8_t *buffer, size_t size);
    enum diag_result (*receive)(void *user, uint8_t *buffer, size_t buffer_size,
                                size_t *bytes_read);
};

struct diag_transport
{
    const struct diag_transport_ops *ops;
    void *user;
};

#endif
