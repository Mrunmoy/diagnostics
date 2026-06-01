#ifndef DIAG_STORAGE_H
#define DIAG_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

typedef struct diag_storage_ops
{
    diag_result_t (*load)(void *user, uint8_t *buffer, size_t buffer_size, size_t *bytes_read);
    diag_result_t (*save)(void *user, const uint8_t *buffer, size_t size);
    diag_result_t (*clear)(void *user);
} diag_storage_ops_t;

typedef struct diag_storage
{
    const diag_storage_ops_t *ops;
    void *user;
} diag_storage_t;

#endif
