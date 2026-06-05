/// @file
/// Storage adapter contract for explicit persistent diagnostics operations.

#ifndef DIAG_STORAGE_H
#define DIAG_STORAGE_H

#include <stddef.h>
#include <stdint.h>

#include "diag/result.h"

/// Whether the storage adapter provides atomic commit semantics.
enum diag_storage_atomic_commit
{
    /// The adapter does not claim atomic commit behavior.
    DIAG_STORAGE_ATOMIC_COMMIT_NONE = 0,
    /// The adapter owns atomic commit or equivalent power-fail handling.
    DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER = 1
};

/// Whether the storage adapter handles wear leveling.
enum diag_storage_wear_leveling
{
    /// The adapter does not claim wear leveling.
    DIAG_STORAGE_WEAR_LEVELING_NONE = 0,
    /// The adapter owns wear leveling for its backing medium.
    DIAG_STORAGE_WEAR_LEVELING_ADAPTER = 1
};

/// Declared properties of a storage adapter.
struct diag_storage_capabilities
{
    /// Erased byte value for the medium, commonly `0xFF` or `0x00`.
    uint8_t erase_value;
    /// Required write size alignment in bytes; must be nonzero.
    size_t write_alignment;
    /// Atomic commit behavior owned by the adapter.
    enum diag_storage_atomic_commit atomic_commit;
    /// Wear-leveling behavior owned by the adapter.
    enum diag_storage_wear_leveling wear_leveling;
};

/// Storage callback table implemented by the platform or downstream project.
///
/// Callbacks receive `struct diag_storage::user` unchanged. They must not retain
/// pointers to temporary buffers after returning unless the downstream adapter
/// contract explicitly owns that lifetime.
struct diag_storage_ops
{
    /// Load serialized bytes into `buffer`, reporting the number of bytes read.
    enum diag_result (*load)(void *user, uint8_t *buffer, size_t buffer_size, size_t *bytes_read);
    /// Save `size` serialized bytes from `buffer`.
    enum diag_result (*save)(void *user, const uint8_t *buffer, size_t size);
    /// Clear or invalidate the adapter's stored diagnostics payload.
    enum diag_result (*clear)(void *user);
};

/// Storage adapter instance.
struct diag_storage
{
    /// Callback table; all callbacks are required for validation.
    const struct diag_storage_ops *ops;
    /// Opaque adapter state passed to every callback.
    void *user;
    /// Medium and adapter capabilities used for validation.
    struct diag_storage_capabilities capabilities;
};

/// Opaque diagnostics context initialized with `diag_init()`.
struct diag_context;

/// Attach a storage adapter to an initialized diagnostics context.
enum diag_result diag_storage_attach(struct diag_context *ctx, const struct diag_storage *storage);

/// Validate storage capability values.
enum diag_result
diag_storage_validate_capabilities(const struct diag_storage_capabilities *capabilities);

/// Validate that a storage adapter has required callbacks and valid capabilities.
enum diag_result diag_storage_validate(const struct diag_storage *storage);

/// Invoke the adapter load callback after validating arguments.
///
/// `*bytes_read` is set to zero before the callback is called.
enum diag_result diag_storage_load(const struct diag_storage *storage, uint8_t *buffer,
                                   size_t buffer_size, size_t *bytes_read);

/// Invoke the adapter save callback after validating arguments and write alignment.
enum diag_result diag_storage_save(const struct diag_storage *storage, const uint8_t *buffer,
                                   size_t size);

/// Invoke the adapter clear callback after validating the adapter.
enum diag_result diag_storage_clear(const struct diag_storage *storage);

/// Save persistent diagnostic state through the attached storage adapter.
///
/// The current C MVP does not yet implement context-level persistence and
/// returns `DIAG_ERROR_NOT_INITIALIZED`.
enum diag_result diag_save(struct diag_context *ctx);

/// Load persistent diagnostic state through the attached storage adapter.
///
/// The current C MVP does not yet implement context-level persistence and
/// returns `DIAG_ERROR_NOT_INITIALIZED`.
enum diag_result diag_load(struct diag_context *ctx);

#endif
