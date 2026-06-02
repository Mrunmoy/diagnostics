#ifndef DIAG_CONTEXT_INTERNAL_H
#define DIAG_CONTEXT_INTERNAL_H

#include <stddef.h>

#include "diag/context.h"

struct diag_context
{
    bool initialized;
    struct diag_config config;
    size_t dtc_count;
};

// Portable C99 alignment probe: the offset of a member placed after a single
// char equals that member type's alignment. Avoids __alignof__, which is a
// GNU/Clang extension and would not compile under MSVC C mode.
struct diag_align_probe_context
{
    char head;
    struct diag_context member;
};

struct diag_align_probe_storage
{
    char head;
    struct diag_context_storage member;
};

typedef char diag_context_storage_size_check
    [(sizeof(struct diag_context) <= DIAG_CONTEXT_STORAGE_SIZE) ? 1 : -1];

// Caller-owned storage must also be aligned strictly enough for the private
// context, not just large enough.
typedef char diag_context_storage_align_check
    [(offsetof(struct diag_align_probe_context, member) <=
      offsetof(struct diag_align_probe_storage, member))
         ? 1
         : -1];

#endif
