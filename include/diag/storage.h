#ifndef DIAG_STORAGE_H
#define DIAG_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

enum diag_storage_atomic_commit
{
    DIAG_STORAGE_ATOMIC_COMMIT_NONE = 0,
    DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER = 1
};

enum diag_storage_wear_leveling
{
    DIAG_STORAGE_WEAR_LEVELING_NONE = 0,
    DIAG_STORAGE_WEAR_LEVELING_ADAPTER = 1
};

struct diag_storage_capabilities
{
    uint8_t erase_value;
    size_t write_alignment;
    enum diag_storage_atomic_commit atomic_commit;
    enum diag_storage_wear_leveling wear_leveling;
};

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
    struct diag_storage_capabilities capabilities;
};

enum diag_result
diag_storage_validate_capabilities(const struct diag_storage_capabilities *capabilities);

enum diag_result diag_storage_validate(const struct diag_storage *storage);

enum diag_result diag_storage_load(const struct diag_storage *storage, uint8_t *buffer,
                                   size_t buffer_size, size_t *bytes_read);

enum diag_result diag_storage_save(const struct diag_storage *storage, const uint8_t *buffer,
                                   size_t size);

enum diag_result diag_storage_clear(const struct diag_storage *storage);

#endif
