#include "diag/diag.h"

#include <stdio.h>

int main(void)
{
    struct diag_context_storage storage = {0};
    struct diag_context        *ctx = NULL;

    const struct diag_config config = {0};

    if (diag_init(&storage, &config, &ctx) != DIAG_OK)
    {
        fprintf(stderr, "diag_init failed\n");
        return 1;
    }

    return 0;
}
