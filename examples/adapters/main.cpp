#include "diag/diag.hpp"

#include <cstdio>

namespace
{

diag::Result exampleLoad(void *, std::uint8_t *, std::size_t, std::size_t &bytesRead) noexcept
{
    bytesRead = 0U;
    return diag::Result::Ok;
}

diag::Result exampleSave(void *, const std::uint8_t *, std::size_t) noexcept
{
    return diag::Result::Ok;
}

diag::Result exampleClear(void *) noexcept
{
    return diag::Result::Ok;
}

diag::Result exampleSend(void *, const std::uint8_t *, std::size_t) noexcept
{
    return diag::Result::Ok;
}

diag::Result exampleReceive(void *, std::uint8_t *, std::size_t, std::size_t &bytesRead) noexcept
{
    bytesRead = 0U;
    return diag::Result::Ok;
}

} // namespace

int main()
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    std::uint8_t         capsule[64]{};

    const diag::Storage storageAdapter{
        diag::StorageOps{exampleLoad, exampleSave, exampleClear},
        nullptr,
        diag::StorageCapabilities{0xFFU, 4U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        capsule,
        sizeof(capsule),
    };
    const diag::Transport transport{diag::TransportOps{exampleSend, exampleReceive}, nullptr};

    if (!context.isInitialized())
    {
        std::fprintf(stderr, "adapters: context init failed\n");
        return 1;
    }

    if (context.attachStorage(storageAdapter) != diag::Result::Ok)
    {
        std::fprintf(stderr, "adapters: storage attach failed\n");
        return 1;
    }

    if (context.attachTransport(transport) != diag::Result::Ok)
    {
        std::fprintf(stderr, "adapters: transport attach failed\n");
        return 1;
    }

    return 0;
}
