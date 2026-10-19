#include "diag/diag.hpp"
#include "example_diag_tool.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

struct ProcessStore
{
    std::uint8_t bytes[256]{};
    std::size_t  used{0U};
};

std::size_t processPersistedSize(void *const user) noexcept
{
    return static_cast<const ProcessStore *>(user)->used;
}

diag::Result processLoad(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                         std::size_t &bytesRead) noexcept
{
    ProcessStore &store = *static_cast<ProcessStore *>(user);
    if (bufferSize < store.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, store.bytes, store.used);
    bytesRead = store.used;
    return diag::Result::Ok;
}

diag::Result processSave(void *const user, const std::uint8_t *const buffer,
                         const std::size_t size) noexcept
{
    ProcessStore &store = *static_cast<ProcessStore *>(user);
    if (size > sizeof(store.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(store.bytes, buffer, size);
    store.used = size;
    return diag::Result::Ok;
}

diag::Result processClear(void *const user) noexcept
{
    static_cast<ProcessStore *>(user)->used = 0U;
    return diag::Result::Ok;
}

} // namespace

int main()
{
    ProcessStore         store{};
    diag::DtcRecord      dtcRecords[3]{};
    diag::ContextStorage storage{};
    diag::Config         config{dtcRecords, 3U, 2U, 10U};
    diag::Context        context{storage, config};
    std::uint8_t         capsuleBuffer[256]{};

    const diag::Identity        identity{diag::EcosystemId{1U},   diag::ProductId{30U},
                                  diag::DeviceType{3U},    diag::DeviceInstance{1U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{1U}};
    const diag::LifecycleConfig lifecycleConfig{diag::ResetCounterPolicy::EveryN, 16U, 0U};
    const diag::Storage         storageAdapter{
        diag::StorageOps{processLoad, processSave, processClear},
        &store,
        diag::StorageCapabilities{0xFFU, 4U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        capsuleBuffer,
        sizeof(capsuleBuffer),
    };

    if (!context.isInitialized() || context.attachIdentity(identity) != diag::Result::Ok ||
        context.attachStorage(storageAdapter) != diag::Result::Ok ||
        context.attachLifecycle(lifecycleConfig) != diag::Result::Ok ||
        context.loadPersistent() != diag::Result::Ok ||
        context.registerFault(diag::LocalFaultId{11U}, diag::DtcId{0x030001U},
                              diag::DtcSeverity::Error) != diag::Result::Ok ||
        context.setFaultTestFailed(diag::LocalFaultId{11U}) != diag::Result::Ok ||
        context.operationCycle() != diag::Result::Ok ||
        context.setFaultTestFailed(diag::LocalFaultId{11U}) != diag::Result::Ok ||
        context.operationCycle() != diag::Result::Ok)
    {
        std::cerr << "process_controller: failed to update process DTC\n";
        return 1;
    }

    const diag::DirtyFlags dirtyFlags = context.dirtyFlags();
    if ((dirtyFlags & static_cast<diag::DirtyFlags>(diag::DirtyFlag::Dtc)) == 0U ||
        context.savePersistent() != diag::Result::Ok)
    {
        std::cerr << "process_controller: expected dirty DTC and successful save\n";
        return 1;
    }

    std::cout << "process_controller: persisted confirmed DTC using " << store.used << " bytes\n";

    const example_diag::Device device{"process_controller", &context, processPersistedSize, &store};
    if (example_diag::runCli(device, diag::DtcId{0x030001U}, std::cout, std::cerr) !=
        diag::Result::Ok)
    {
        std::cerr << "process_controller: diagnostic tool flow failed\n";
        return 1;
    }

    return 0;
}
