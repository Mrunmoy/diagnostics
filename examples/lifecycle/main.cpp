#include "diag/diag.hpp"

#include <cstdint>
#include <iostream>

namespace
{

int fail(const char *const message)
{
    std::cerr << "lifecycle_example: " << message << '\n';
    return 1;
}

} // namespace

int main()
{
    diag::ContextStorage storage{};
    diag::Context        diagnostics{storage};

    const diag::LifecycleConfig lifecycleConfig{
        diag::ResetCounterPolicy::AbnormalOnly,
        0U,
        0U,
    };

    if (diagnostics.attachLifecycle(lifecycleConfig) != diag::Result::Ok)
    {
        return fail("failed to attach lifecycle counters");
    }

    if (diagnostics.observeReset(diag::ResetReason::PowerOn) != diag::Result::Ok)
    {
        return fail("failed to observe power-on reset");
    }

    if (diagnostics.observeReset(diag::ResetReason::Watchdog) != diag::Result::Ok)
    {
        return fail("failed to observe watchdog reset");
    }

    const diag::ResultValue<diag::LifecycleSnapshot> snapshot = diagnostics.lifecycle();
    if (!snapshot.hasValue())
    {
        return fail("failed to read lifecycle snapshot");
    }

    std::cout << "lifecycle_example: reset_count=" << snapshot.value().resetCount
              << " abnormal_reset_count=" << snapshot.value().abnormalResetCount
              << " persist_requested=" << (snapshot.value().persistRequested ? "yes" : "no")
              << '\n';

    if (snapshot.value().resetCount != 2U || snapshot.value().abnormalResetCount != 1U ||
        !snapshot.value().persistRequested)
    {
        return fail("unexpected lifecycle counters");
    }

    return 0;
}
