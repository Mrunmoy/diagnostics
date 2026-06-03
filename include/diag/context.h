/// @file
/// Diagnostics context configuration and lifetime APIs.

#ifndef DIAG_CONTEXT_H
#define DIAG_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/compiler.h"
#include "diag/dtc.h"
#include "diag/identity.h"
#include "diag/lifecycle.h"
#include "diag/result.h"
#include "diag/storage.h"
#include "diag/transport.h"

/// Size in bytes reserved for the private diagnostics context.
///
/// Callers allocate this storage, usually statically. The implementation has a
/// compile-time check that fails the build if the private context outgrows this
/// public fixed budget.
#define DIAG_CONTEXT_STORAGE_SIZE (160u)

/// Required alignment in bytes for `struct diag_context_storage`.
#define DIAG_CONTEXT_STORAGE_ALIGN (8u)

/// Context initialization configuration.
///
/// All runtime memory is caller-owned. The DTC buffer must remain valid until
/// `diag_deinit()` completes. Storage and transport adapters are copied by
/// value; any adapter-specific state is owned through the supplied `user`
/// pointers.
struct diag_config
{
    /// Fixed array used by the context for registered DTC records.
    struct diag_dtc_snapshot *dtc_buffer;
    /// Number of entries available in `dtc_buffer`; must be nonzero.
    size_t dtc_capacity;
    /// Optional persistent storage adapter configuration.
    struct diag_storage storage;
    /// Optional transport adapter configuration.
    struct diag_transport transport;
    /// Compact numeric device identity copied into the context.
    struct diag_identity identity;
    /// Lifecycle/reset counter policy.
    struct diag_lifecycle_config lifecycle;
    /// DTC operation-cycle thresholds.
    struct diag_dtc_config dtc;
};

/// Opaque diagnostics context returned by `diag_init()`.
struct diag_context;

/// Caller-owned storage for one opaque diagnostics context.
///
/// Allocate this object with static, automatic, or caller-managed storage. The
/// library does not allocate a context from the heap.
struct diag_context_storage
{
    /// Private context bytes; callers must not inspect or persist this raw layout.
    DIAG_ALIGNAS_PREFIX(DIAG_CONTEXT_STORAGE_ALIGN)
    uint8_t bytes[DIAG_CONTEXT_STORAGE_SIZE] DIAG_ALIGNAS_SUFFIX(DIAG_CONTEXT_STORAGE_ALIGN);
};

/// Initialize a diagnostics context in caller-owned storage.
///
/// On success, `*out_ctx` points into `context_storage`. On failure, `*out_ctx`
/// is cleared when the pointer itself is valid. The DTC buffer in `config` must
/// remain alive and writable for the lifetime of the context.
enum diag_result diag_init(struct diag_context_storage *context_storage,
                           const struct diag_config *config, struct diag_context **out_ctx);

/// Deinitialize a context previously returned by `diag_init()`.
///
/// This clears library runtime state in the private context. It does not free or
/// modify adapter-owned resources and does not write persistent storage.
enum diag_result diag_deinit(struct diag_context *ctx);

/// Save persistent diagnostic state through the configured storage adapter.
///
/// The current C MVP does not yet implement context-level persistence and
/// returns `DIAG_ERROR_NOT_INITIALIZED`.
enum diag_result diag_save(struct diag_context *ctx);

/// Load persistent diagnostic state through the configured storage adapter.
///
/// The current C MVP does not yet implement context-level persistence and
/// returns `DIAG_ERROR_NOT_INITIALIZED`.
enum diag_result diag_load(struct diag_context *ctx);

#endif
