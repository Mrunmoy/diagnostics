#ifndef DIAG_STORAGE_H
#define DIAG_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

struct diag_storage_ops
{
    enum diag_result (*load)(void *user, uint8_t *buffer, size_t buffer_size, size_t *bytes_read);
    enum diag_result (*save)(void *user, const uint8_t *buffer, size_t size);
    enum diag_result (*clear)(void *user);
};

struct diag_storage
{
    const struct diag_storage_ops *ops;
    void *user;
};

#endif
