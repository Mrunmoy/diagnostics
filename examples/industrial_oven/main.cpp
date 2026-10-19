#include "diag/diag.hpp"
#include "example_diag_tool.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

struct FakeStorage
{
    std::uint8_t bytes[256]{};
    std::size_t  used{0U};
};

std::size_t ovenPersistedSize(void *const user) noexcept
{
    return static_cast<const FakeStorage *>(user)->used;
}

diag::Result fakeLoad(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                      std::size_t &bytesRead) noexcept
{
    FakeStorage &fake = *static_cast<FakeStorage *>(user);
    if (bufferSize < fake.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, fake.bytes, fake.used);
    bytesRead = fake.used;
    return diag::Result::Ok;
}

diag::Result fakeSave(void *const user, const std::uint8_t *const buffer,
                      const std::size_t size) noexcept
{
    FakeStorage &fake = *static_cast<FakeStorage *>(user);
    if (size > sizeof(fake.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(fake.bytes, buffer, size);
    fake.used = size;
    return diag::Result::Ok;
}

diag::Result fakeClear(void *const user) noexcept
{
    static_cast<FakeStorage *>(user)->used = 0U;
    return diag::Result::Ok;
}

} // namespace

int main()
{
    FakeStorage          fake{};
    diag::DtcRecord      dtcRecords[4]{};
    diag::ContextStorage storage{};
    diag::Config         config{dtcRecords, 4U};
    diag::Context        context{storage, config};
    std::uint8_t         capsuleBuffer[256]{};

    const diag::Identity        identity{diag::EcosystemId{1U},   diag::ProductId{20U},
                                  diag::DeviceType{2U},    diag::DeviceInstance{1U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{1U}};
    const diag::LifecycleConfig lifecycleConfig{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};
    const diag::Storage         persistentStorage{
        diag::StorageOps{fakeLoad, fakeSave, fakeClear},
        &fake,
        diag::StorageCapabilities{0xFFU, 4U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        capsuleBuffer,
        sizeof(capsuleBuffer),
    };

    if (!context.isInitialized() || context.attachIdentity(identity) != diag::Result::Ok ||
        context.attachStorage(persistentStorage) != diag::Result::Ok ||
        context.attachLifecycle(lifecycleConfig) != diag::Result::Ok ||
        context.loadPersistent() != diag::Result::Ok ||
        context.registerFault(diag::LocalFaultId{100U}, diag::DtcId{0x020001U},
                              diag::DtcSeverity::Critical) != diag::Result::Ok ||
        context.setFaultTestFailed(diag::LocalFaultId{100U}) != diag::Result::Ok ||
        context.operationCycle() != diag::Result::Ok ||
        context.observeReset(diag::ResetReason::Watchdog) != diag::Result::Ok ||
        context.savePersistent() != diag::Result::Ok)
    {
        std::cerr << "industrial_oven: failed to save diagnostics\n";
        return 1;
    }

    std::memset(dtcRecords, 0, sizeof(dtcRecords));
    if (context.loadPersistent() != diag::Result::Ok)
    {
        std::cerr << "industrial_oven: failed to restore diagnostics\n";
        return 1;
    }

    const diag::ResultValue<diag::DtcRecord> restoredFault = context.dtc(diag::DtcId{0x020001U});
    if (!restoredFault.hasValue())
    {
        std::cerr << "industrial_oven: failed to restore diagnostics\n";
        return 1;
    }

    std::cout << "industrial_oven: persisted DTC 0x" << std::hex << restoredFault.value().id.value
              << std::dec << " in " << fake.used << " bytes\n";

    const example_diag::Device device{"industrial_oven", &context, ovenPersistedSize, &fake};
    if (example_diag::runCli(device, diag::DtcId{0x020001U}, std::cout, std::cerr) !=
        diag::Result::Ok)
    {
        std::cerr << "industrial_oven: diagnostic tool flow failed\n";
        return 1;
    }

    return 0;
}
