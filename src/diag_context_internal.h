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

#endif
