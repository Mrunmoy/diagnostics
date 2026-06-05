#include "diag/storage.h"

#include "diag_context_internal.h"

static int diag_storage_size_is_aligned(size_t size, size_t alignment)
{
    return (size % alignment) == 0u;
}

enum diag_result diag_storage_attach(struct diag_context *ctx, const struct diag_storage *storage)
{
    enum diag_result result;

    if (ctx == NULL || storage == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (!diag_context_has_state(ctx, DIAG_CONTEXT_STATE_INITIALIZED))
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    result = diag_storage_validate(storage);
    if (result != DIAG_OK)
    {
        return result;
    }

    ctx->storage = *storage;
    diag_context_set_state(ctx, DIAG_CONTEXT_STATE_STORAGE_ATTACHED);

    return DIAG_OK;
}

enum diag_result
diag_storage_validate_capabilities(const struct diag_storage_capabilities *capabilities)
{
    if (capabilities == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (capabilities->write_alignment == 0u)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if ((capabilities->atomic_commit != DIAG_STORAGE_ATOMIC_COMMIT_NONE) &&
        (capabilities->atomic_commit != DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER))
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if ((capabilities->wear_leveling != DIAG_STORAGE_WEAR_LEVELING_NONE) &&
        (capabilities->wear_leveling != DIAG_STORAGE_WEAR_LEVELING_ADAPTER))
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    return DIAG_OK;
}

enum diag_result diag_storage_validate(const struct diag_storage *storage)
{
    if ((storage == NULL) || (storage->ops == NULL) || (storage->ops->load == NULL) ||
        (storage->ops->save == NULL) || (storage->ops->clear == NULL))
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    return diag_storage_validate_capabilities(&storage->capabilities);
}

enum diag_result diag_storage_load(const struct diag_storage *storage, uint8_t *buffer,
                                   size_t buffer_size, size_t *bytes_read)
{
    enum diag_result result = diag_storage_validate(storage);

    if (result != DIAG_OK)
    {
        return result;
    }

    if ((buffer == NULL) || (buffer_size == 0u) || (bytes_read == NULL))
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    *bytes_read = 0u;
    return storage->ops->load(storage->user, buffer, buffer_size, bytes_read);
}

enum diag_result diag_storage_save(const struct diag_storage *storage, const uint8_t *buffer,
                                   size_t size)
{
    enum diag_result result = diag_storage_validate(storage);

    if (result != DIAG_OK)
    {
        return result;
    }

    if ((buffer == NULL) || (size == 0u) ||
        !diag_storage_size_is_aligned(size, storage->capabilities.write_alignment))
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    return storage->ops->save(storage->user, buffer, size);
}

enum diag_result diag_storage_clear(const struct diag_storage *storage)
{
    enum diag_result result = diag_storage_validate(storage);

    if (result != DIAG_OK)
    {
        return result;
    }

    return storage->ops->clear(storage->user);
}

enum diag_result diag_save(struct diag_context *ctx)
{
    if (ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (!diag_context_has_state(ctx, DIAG_CONTEXT_STATE_INITIALIZED |
                                         DIAG_CONTEXT_STATE_STORAGE_ATTACHED))
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return DIAG_ERROR_NOT_INITIALIZED;
}

enum diag_result diag_load(struct diag_context *ctx)
{
    if (ctx == NULL)
    {
        return DIAG_ERROR_INVALID_ARGUMENT;
    }

    if (!diag_context_has_state(ctx, DIAG_CONTEXT_STATE_INITIALIZED |
                                         DIAG_CONTEXT_STATE_STORAGE_ATTACHED))
    {
        return DIAG_ERROR_NOT_INITIALIZED;
    }

    return DIAG_ERROR_NOT_INITIALIZED;
}
