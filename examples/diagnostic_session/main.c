#include "device.h"

#include <stdio.h>

int main(void)
{
    struct diagnostic_device device = {0};
    enum diag_result         result = diagnostic_device_init(&device);

    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_session: device setup failed: %d\n", (int)result);
        return 1;
    }

    {
        const struct example_diag_device endpoint = {
            .name = "diagnostic_session",
            .ctx = device.ctx,
            .persisted_size = diagnostic_device_persisted_size,
            .user = &device,
        };

        result = example_diag_tool_run_cli(&endpoint, 0x030101u);
    }

    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_session: tester failed: %d\n", (int)result);
        (void)diagnostic_device_deinit(&device);
        return 1;
    }

    result = diagnostic_device_deinit(&device);
    if (result != DIAG_OK)
    {
        fprintf(stderr, "diagnostic_session: device cleanup failed: %d\n", (int)result);
        return 1;
    }

    return 0;
}
