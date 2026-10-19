#include "diag/diag.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>

namespace
{

struct CapsuleBank
{
    std::uint8_t bytes[192]{};
    std::size_t  used{0U};
};

diag::Result bankLoad(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                      std::size_t &bytesRead) noexcept
{
    CapsuleBank &bank = *static_cast<CapsuleBank *>(user);
    if (bufferSize < bank.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, bank.bytes, bank.used);
    bytesRead = bank.used;
    return diag::Result::Ok;
}

diag::Result bankSave(void *const user, const std::uint8_t *const buffer,
                      const std::size_t size) noexcept
{
    CapsuleBank &bank = *static_cast<CapsuleBank *>(user);
    if (size > sizeof(bank.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(bank.bytes, buffer, size);
    bank.used = size;
    return diag::Result::Ok;
}

diag::Result bankClear(void *const user) noexcept
{
    static_cast<CapsuleBank *>(user)->used = 0U;
    return diag::Result::Ok;
}

diag::Storage makeBankStorage(CapsuleBank &bank, std::uint8_t *const buffer,
                              const std::size_t bufferSize) noexcept
{
    return diag::Storage{
        diag::StorageOps{bankLoad, bankSave, bankClear},
        &bank,
        diag::StorageCapabilities{0xFFU, 4U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        buffer,
        bufferSize,
    };
}

[[nodiscard]] diag::Result saveBootloaderBank(CapsuleBank &bank) noexcept
{
    diag::ContextStorage storage{};
    diag::Context        context{storage};
    std::uint8_t         capsuleBuffer[192]{};
    const diag::Storage  bankStorage = makeBankStorage(bank, capsuleBuffer, sizeof(capsuleBuffer));
    const diag::LifecycleConfig lifecycleConfig{diag::ResetCounterPolicy::EveryN, 1U, 0U};

    if (!context.isInitialized() || context.attachStorage(bankStorage) != diag::Result::Ok ||
        context.attachLifecycle(lifecycleConfig) != diag::Result::Ok ||
        context.loadPersistent() != diag::Result::Ok ||
        context.observeReset(diag::ResetReason::Update) != diag::Result::Ok ||
        context.savePersistent() != diag::Result::Ok)
    {
        return diag::Result::Storage;
    }

    return diag::Result::Ok;
}

[[nodiscard]] diag::Result saveApplicationBank(CapsuleBank &bank) noexcept
{
    diag::DtcRecord      dtcRecords[2]{};
    diag::ContextStorage storage{};
    diag::Config         config{dtcRecords, 2U};
    diag::Context        context{storage, config};
    std::uint8_t         capsuleBuffer[192]{};
    const diag::Storage  bankStorage = makeBankStorage(bank, capsuleBuffer, sizeof(capsuleBuffer));

    if (!context.isInitialized() || context.attachStorage(bankStorage) != diag::Result::Ok ||
        context.loadPersistent() != diag::Result::Ok ||
        context.registerFault(diag::LocalFaultId{9U}, diag::DtcId{0x060001U},
                              diag::DtcSeverity::Error) != diag::Result::Ok ||
        context.setFaultTestFailed(diag::LocalFaultId{9U}) != diag::Result::Ok ||
        context.operationCycle() != diag::Result::Ok ||
        context.savePersistent() != diag::Result::Ok)
    {
        return diag::Result::Storage;
    }

    return diag::Result::Ok;
}

} // namespace

int main()
{
    CapsuleBank bootloaderBank{};
    CapsuleBank applicationBank{};

    if (saveBootloaderBank(bootloaderBank) != diag::Result::Ok ||
        saveApplicationBank(applicationBank) != diag::Result::Ok)
    {
        std::cerr << "bootloader_app_shared: save failed\n";
        return 1;
    }

    if (bootloaderBank.used == 0U || applicationBank.used == 0U)
    {
        std::cerr << "bootloader_app_shared: expected two independent banks\n";
        return 1;
    }

    std::cout << "bootloader_app_shared: bootloader bank " << bootloaderBank.used
              << " bytes, application bank " << applicationBank.used << " bytes\n";
    return 0;
}
