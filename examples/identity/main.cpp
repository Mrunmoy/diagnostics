#include "diag/diag.hpp"

#include <cstdio>

int main()
{
    diag::ContextStorage storage{};
    diag::Context        diagnostics{storage};

    const diag::Identity identity{
        diag::EcosystemId{7U},
        diag::ProductId{90U},
        diag::DeviceType{4U},
        diag::DeviceInstance{3U},
        diag::FirmwareStage{1U},
        diag::FirmwareComponent{1U},
        0U,
    };

    if (diagnostics.attachIdentity(identity) != diag::Result::Ok)
    {
        std::fprintf(stderr, "identity_example: failed to attach identity\n");
        return 1;
    }

    const diag::ResultValue<diag::Identity> configured = diagnostics.identity();
    if (!configured.hasValue())
    {
        std::fprintf(stderr, "identity_example: identity query failed\n");
        return 2;
    }

    std::printf(
        "identity_example: ecosystem=%u product=%u type=%u instance=%u stage=%u component=%u\n",
        static_cast<unsigned>(configured.value().ecosystem.value),
        static_cast<unsigned>(configured.value().product.value),
        static_cast<unsigned>(configured.value().deviceType.value),
        static_cast<unsigned>(configured.value().instance.value),
        static_cast<unsigned>(configured.value().firmwareStage.value),
        static_cast<unsigned>(configured.value().firmwareComponent.value));

    return 0;
}
