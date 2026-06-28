#include "diag/diag.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace
{

constexpr std::size_t kStorageBytes = 256U;

struct MemoryStore
{
    std::array<std::uint8_t, kStorageBytes> bytes{};
    std::size_t                             length{0U};
};

diag::Result loadBytes(void *const user, std::uint8_t *const buffer, const std::size_t capacity,
                       std::size_t &bytesRead) noexcept
{
    MemoryStore &store = *static_cast<MemoryStore *>(user);
    bytesRead = 0U;

    if (capacity < store.length)
    {
        return diag::Result::Capacity;
    }

    for (std::size_t index = 0U; index < store.length; ++index)
    {
        buffer[index] = store.bytes[index];
    }

    bytesRead = store.length;
    return diag::Result::Ok;
}

diag::Result saveBytes(void *const user, const std::uint8_t *const buffer,
                       const std::size_t length) noexcept
{
    MemoryStore &store = *static_cast<MemoryStore *>(user);
    if (length > store.bytes.size())
    {
        return diag::Result::Capacity;
    }

    for (std::size_t index = 0U; index < length; ++index)
    {
        store.bytes[index] = buffer[index];
    }

    store.length = length;
    return diag::Result::Ok;
}

diag::Result clearBytes(void *const user) noexcept
{
    MemoryStore &store = *static_cast<MemoryStore *>(user);
    store.length = 0U;
    return diag::Result::Ok;
}

int fail(const char *const message)
{
    std::cerr << "storage_example: " << message << '\n';
    return 1;
}

diag::Storage makeStorage(MemoryStore &store, std::array<std::uint8_t, kStorageBytes> &capsule)
{
    return diag::Storage{
        diag::StorageOps{loadBytes, saveBytes, clearBytes},
        &store,
        diag::StorageCapabilities{0xFFU, 8U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        capsule.data(),
        capsule.size(),
    };
}

} // namespace

int main()
{
    MemoryStore                             persistentStore{};
    std::array<std::uint8_t, kStorageBytes> saveCapsule{};
    std::array<std::uint8_t, kStorageBytes> loadCapsule{};
    std::array<diag::DtcRecord, 2U>         sourceRecords{};
    std::array<diag::DtcRecord, 2U>         restoredRecords{};
    diag::ContextStorage                    sourceStorage{};
    diag::ContextStorage                    restoredStorage{};
    diag::Context source{sourceStorage, diag::Config{sourceRecords.data(), sourceRecords.size()}};
    diag::Context restored{restoredStorage,
                           diag::Config{restoredRecords.data(), restoredRecords.size()}};

    if (source.attachStorage(makeStorage(persistentStore, saveCapsule)) != diag::Result::Ok)
    {
        return fail("failed to attach save storage");
    }

    if (source.registerDtc(diag::DtcId{0x040101U}, diag::DtcSeverity::Critical) != diag::Result::Ok)
    {
        return fail("failed to register DTC");
    }

    if (source.setDtcActive(diag::DtcId{0x040101U}, true) != diag::Result::Ok)
    {
        return fail("failed to activate DTC");
    }

    if (source.savePersistent() != diag::Result::Ok)
    {
        return fail("failed to save persistent diagnostics");
    }

    if (restored.attachStorage(makeStorage(persistentStore, loadCapsule)) != diag::Result::Ok)
    {
        return fail("failed to attach load storage");
    }

    if (restored.loadPersistent() != diag::Result::Ok)
    {
        return fail("failed to load persistent diagnostics");
    }

    const diag::ResultValue<diag::DtcRecord> record = restored.dtc(diag::DtcId{0x040101U});
    if (!record.hasValue() || !diag::hasStatus(record.value(), diag::DtcStatus::TestFailed))
    {
        return fail("restored DTC is missing or inactive");
    }

    std::cout << "storage_example: restored dtc=0x" << std::hex << record.value().id.value
              << " persisted_bytes=" << std::dec << persistentStore.length << '\n';

    return 0;
}
