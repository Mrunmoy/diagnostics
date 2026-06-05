#include "diag/diag.h"

int main(void)
{
    struct diag_context_storage storage = {0};
    struct diag_context        *ctx = NULL;

    const struct diag_config config = {0};

    return diag_init(&storage, &config, &ctx) == DIAG_OK ? 0 : 1;
}
