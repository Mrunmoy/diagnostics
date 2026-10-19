#include "diag/diag.hpp"
#include "example_diag_tool.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

struct EcuStore
{
    std::uint8_t bytes[256]{};
    std::size_t  used{0U};
};

struct EcuBus
{
    std::uint8_t lastFrame[32]{};
    std::size_t  lastFrameSize{0U};
};

std::size_t ecuPersistedSize(void *const user) noexcept
{
    return static_cast<const EcuStore *>(user)->used;
}

diag::Result ecuLoad(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                     std::size_t &bytesRead) noexcept
{
    EcuStore &store = *static_cast<EcuStore *>(user);
    if (bufferSize < store.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, store.bytes, store.used);
    bytesRead = store.used;
    return diag::Result::Ok;
}

diag::Result ecuSave(void *const user, const std::uint8_t *const buffer,
                     const std::size_t size) noexcept
{
    EcuStore &store = *static_cast<EcuStore *>(user);
    if (size > sizeof(store.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(store.bytes, buffer, size);
    store.used = size;
    return diag::Result::Ok;
}

diag::Result ecuClear(void *const user) noexcept
{
    static_cast<EcuStore *>(user)->used = 0U;
    return diag::Result::Ok;
}

diag::Result ecuSend(void *const user, const std::uint8_t *const buffer,
                     const std::size_t size) noexcept
{
    EcuBus &bus = *static_cast<EcuBus *>(user);
    if (size > sizeof(bus.lastFrame))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(bus.lastFrame, buffer, size);
    bus.lastFrameSize = size;
    return diag::Result::Ok;
}

diag::Result ecuReceive(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                        std::size_t &bytesRead) noexcept
{
    EcuBus &bus = *static_cast<EcuBus *>(user);
    if (bufferSize < bus.lastFrameSize)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, bus.lastFrame, bus.lastFrameSize);
    bytesRead = bus.lastFrameSize;
    return diag::Result::Ok;
}

} // namespace

int main()
{
    EcuStore             store{};
    EcuBus               bus{};
    diag::DtcRecord      dtcRecords[4]{};
    diag::ContextStorage storage{};
    diag::Config         config{dtcRecords, 4U};
    diag::Context        context{storage, config};
    std::uint8_t         capsuleBuffer[256]{};

    const diag::Identity        identity{diag::EcosystemId{1U},   diag::ProductId{50U},
                                  diag::DeviceType{5U},    diag::DeviceInstance{1U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{1U}};
    const diag::LifecycleConfig lifecycleConfig{diag::ResetCounterPolicy::AbnormalOnly, 0U, 0U};
    const diag::Storage         storageAdapter{
        diag::StorageOps{ecuLoad, ecuSave, ecuClear},
        &store,
        diag::StorageCapabilities{0xFFU, 8U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        capsuleBuffer,
        sizeof(capsuleBuffer),
    };
    const diag::Transport transport{diag::TransportOps{ecuSend, ecuReceive}, &bus};

    if (!context.isInitialized() || context.attachIdentity(identity) != diag::Result::Ok ||
        context.attachStorage(storageAdapter) != diag::Result::Ok ||
        context.attachTransport(transport) != diag::Result::Ok ||
        context.attachLifecycle(lifecycleConfig) != diag::Result::Ok ||
        context.loadPersistent() != diag::Result::Ok ||
        context.registerFault(diag::LocalFaultId{77U}, diag::DtcId{0x050001U},
                              diag::DtcSeverity::Error) != diag::Result::Ok ||
        context.setFaultTestFailed(diag::LocalFaultId{77U}) != diag::Result::Ok ||
        context.operationCycle() != diag::Result::Ok)
    {
        std::cerr << "ecu_node: diagnostic flow failed\n";
        return 1;
    }

    const diag::ResultValue<diag::DtcRecord> fault = context.dtc(diag::DtcId{0x050001U});
    if (!fault.hasValue() || context.savePersistent() != diag::Result::Ok)
    {
        std::cerr << "ecu_node: save failed\n";
        return 1;
    }

    if (!diag::hasStatus(fault.value(), diag::DtcStatus::Confirmed))
    {
        std::cerr << "ecu_node: expected confirmed DTC\n";
        return 1;
    }

    std::cout << "ecu_node: confirmed DTC 0x" << std::hex << fault.value().id.value << std::dec
              << ", saved " << store.used << " bytes\n";

    const example_diag::Device device{"ecu_node", &context, ecuPersistedSize, &store};
    if (example_diag::runCli(device, diag::DtcId{0x050001U}, std::cout, std::cerr) !=
        diag::Result::Ok)
    {
        std::cerr << "ecu_node: diagnostic tool flow failed\n";
        return 1;
    }

    return 0;
}
