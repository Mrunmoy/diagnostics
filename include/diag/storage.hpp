#pragma once

#include "diag/result.hpp"

#include <cstddef>
#include <cstdint>

namespace diag
{

enum class StorageAtomicCommit : std::uint8_t
{
    None = 0U,
    Adapter,
};

enum class StorageWearLeveling : std::uint8_t
{
    None = 0U,
    Adapter,
};

struct StorageCapabilities
{
    std::uint8_t        eraseValue{0xFFU};
    std::size_t         writeAlignment{1U};
    StorageAtomicCommit atomicCommit{StorageAtomicCommit::None};
    StorageWearLeveling wearLeveling{StorageWearLeveling::None};
};

using StorageLoad = Result (*)(void *user, std::uint8_t *buffer, std::size_t capacity,
                               std::size_t &bytesRead) noexcept;
using StorageSave = Result (*)(void *user, const std::uint8_t *buffer, std::size_t length) noexcept;
using StorageClear = Result (*)(void *user) noexcept;

struct StorageOps
{
    StorageLoad  load{nullptr};
    StorageSave  save{nullptr};
    StorageClear clear{nullptr};
};

struct Storage
{
    StorageOps          ops{};
    void               *user{nullptr};
    StorageCapabilities capabilities{};
    std::uint8_t       *capsuleBuffer{nullptr};
    std::size_t         capsuleBufferSize{0U};
};

[[nodiscard]] Result validateStorageCapabilities(const StorageCapabilities &capabilities) noexcept;
[[nodiscard]] Result validateStorage(const Storage &storage) noexcept;
[[nodiscard]] Result storageLoad(const Storage &storage, std::uint8_t *buffer, std::size_t capacity,
                                 std::size_t &bytesRead) noexcept;
[[nodiscard]] Result storageSave(const Storage &storage, const std::uint8_t *buffer,
                                 std::size_t length) noexcept;
[[nodiscard]] Result storageClear(const Storage &storage) noexcept;

} // namespace diag
