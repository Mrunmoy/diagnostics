#include "diag/diag.h"

int main(void)
{
    diag_context_t ctx;
    diag_dtc_snapshot_t dtc_buffer[8];

    const diag_config_t config = {
        .dtc_buffer = dtc_buffer,
        .dtc_capacity = 8,
        .storage = {0},
        .transport = {0},
    };

    return diag_init(&ctx, &config) == DIAG_OK ? 0 : 1;
}
