/// @file
/// Diagnostics context configuration and lifetime APIs.

#ifndef DIAG_CONTEXT_H
#define DIAG_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diag/compiler.h"
#include "diag/result.h"

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
/// The core context has no feature-specific memory. Optional modules attach
/// their own caller-owned resources through their module APIs after
/// `diag_init()` succeeds.
struct diag_config
{
    /// Reserved for future core options; initialize to zero.
    uint8_t reserved;
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
/// is cleared when the pointer itself is valid. Optional features are attached
/// through feature-specific APIs after the core context is initialized.
enum diag_result diag_init(struct diag_context_storage *context_storage,
                           const struct diag_config *config, struct diag_context **out_ctx);

/// Deinitialize a context previously returned by `diag_init()`.
///
/// This clears library runtime state in the private context. It does not free or
/// modify adapter-owned resources and does not write persistent storage.
enum diag_result diag_deinit(struct diag_context *ctx);

#endif
