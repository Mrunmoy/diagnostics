#include "diag/diag.h"

int main(void)
{
    struct diag_context_storage storage = {0};
    struct diag_context *ctx = 0;
    struct diag_dtc_snapshot dtc_buffer[8];

    const struct diag_config config = {
        .dtc_buffer = dtc_buffer,
        .dtc_capacity = 8,
        .storage = {0},
        .transport = {0},
    };

    return diag_init(&storage, &config, &ctx) == DIAG_OK ? 0 : 1;
}
