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

    result = diagnostic_tester_run(&device);
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
