#ifndef DIAG_CONTEXT_INTERNAL_H
#define DIAG_CONTEXT_INTERNAL_H

#include "diag/context.h"

struct diag_context
{
    bool initialized;
    struct diag_config config;
    size_t dtc_count;
};

typedef char diag_context_storage_size_check
    [(sizeof(struct diag_context) <= DIAG_CONTEXT_STORAGE_SIZE) ? 1 : -1];

// Caller-owned storage must also be aligned strictly enough for the private
// context, not just large enough. __alignof__ is a compiler builtin (this is an
// internal, library-only header), so the guard stays C99-clean under -pedantic.
typedef char diag_context_storage_align_check
    [(__alignof__(struct diag_context) <= __alignof__(struct diag_context_storage)) ? 1 : -1];

#endif
