#include "diag/storage.hpp"

namespace diag
{

namespace
{

[[nodiscard]] bool isAligned(const std::size_t value, const std::size_t alignment) noexcept
{
    return (value % alignment) == 0U;
}

} // namespace

Result validateStorageCapabilities(const StorageCapabilities &capabilities) noexcept
{
    if (capabilities.writeAlignment == 0U)
    {
        return Result::InvalidArgument;
    }

    if (capabilities.atomicCommit != StorageAtomicCommit::None &&
        capabilities.atomicCommit != StorageAtomicCommit::Adapter)
    {
        return Result::InvalidArgument;
    }

    if (capabilities.wearLeveling != StorageWearLeveling::None &&
        capabilities.wearLeveling != StorageWearLeveling::Adapter)
    {
        return Result::InvalidArgument;
    }

    return Result::Ok;
}

Result validateStorage(const Storage &storage) noexcept
{
    if (storage.ops.load == nullptr || storage.ops.save == nullptr || storage.ops.clear == nullptr)
    {
        return Result::InvalidArgument;
    }

    return validateStorageCapabilities(storage.capabilities);
}

Result storageLoad(const Storage &storage, std::uint8_t *const buffer, const std::size_t capacity,
                   std::size_t &bytesRead) noexcept
{
    const Result validation = validateStorage(storage);
    bytesRead = 0U;

    if (validation != Result::Ok)
    {
        return validation;
    }

    if (buffer == nullptr || capacity == 0U)
    {
        return Result::InvalidArgument;
    }

    const Result result = storage.ops.load(storage.user, buffer, capacity, bytesRead);
    if (result == Result::Ok && bytesRead > capacity)
    {
        bytesRead = 0U;
        return Result::Storage;
    }

    return result;
}

Result storageSave(const Storage &storage, const std::uint8_t *const buffer,
                   const std::size_t length) noexcept
{
    const Result validation = validateStorage(storage);
    if (validation != Result::Ok)
    {
        return validation;
    }

    if (buffer == nullptr || length == 0U ||
        !isAligned(length, storage.capabilities.writeAlignment))
    {
        return Result::InvalidArgument;
    }

    return storage.ops.save(storage.user, buffer, length);
}

Result storageClear(const Storage &storage) noexcept
{
    const Result validation = validateStorage(storage);
    if (validation != Result::Ok)
    {
        return validation;
    }

    return storage.ops.clear(storage.user);
}

} // namespace diag
