#include "diag/storage.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <gtest/gtest.h>

namespace
{

constexpr std::size_t kStorageBytes = 256U;

struct MemoryStorage
{
    std::array<std::uint8_t, kStorageBytes> persisted{};
    std::size_t                             loadCalls{0U};
    std::size_t                             saveCalls{0U};
    std::size_t                             clearCalls{0U};
};

diag::Result memoryLoad(void *const  user, std::uint8_t *const, const std::size_t,
                        std::size_t &bytesRead) noexcept
{
    MemoryStorage &storage = *static_cast<MemoryStorage *>(user);
    ++storage.loadCalls;
    bytesRead = 0U;
    return diag::Result::Ok;
}

diag::Result memorySave(void *const user, const std::uint8_t *const, const std::size_t) noexcept
{
    MemoryStorage &storage = *static_cast<MemoryStorage *>(user);
    ++storage.saveCalls;
    return diag::Result::Ok;
}

diag::Result memoryClear(void *const user) noexcept
{
    MemoryStorage &storage = *static_cast<MemoryStorage *>(user);
    ++storage.clearCalls;
    return diag::Result::Ok;
}

diag::Result oversizedLoad(void *, std::uint8_t *, const std::size_t capacity,
                           std::size_t &bytesRead) noexcept
{
    bytesRead = capacity + 1U;
    return diag::Result::Ok;
}

} // namespace

TEST(DiagStorage, ValidatesCallbacksAndCapabilities)
{
    MemoryStorage                           memory{};
    std::array<std::uint8_t, kStorageBytes> capsule{};

    diag::Storage storage{diag::StorageOps{memoryLoad, memorySave, memoryClear},
                          &memory,
                          {},
                          capsule.data(),
                          capsule.size()};
    EXPECT_EQ(diag::validateStorage(storage), diag::Result::Ok);

    storage.ops.load = nullptr;
    EXPECT_EQ(diag::validateStorage(storage), diag::Result::InvalidArgument);

    storage.ops.load = memoryLoad;
    storage.capabilities.writeAlignment = 0U;
    EXPECT_EQ(diag::validateStorage(storage), diag::Result::InvalidArgument);
}

TEST(DiagStorage, LoadRejectsAdapterReportedOverflow)
{
    MemoryStorage                           memory{};
    std::array<std::uint8_t, kStorageBytes> buffer{};
    std::size_t                             bytesRead = 7U;
    const diag::Storage                     storage{
        diag::StorageOps{oversizedLoad, memorySave, memoryClear}, &memory, {}, nullptr, 0U};

    EXPECT_EQ(diag::storageLoad(storage, buffer.data(), buffer.size(), bytesRead),
              diag::Result::Storage);
    EXPECT_EQ(bytesRead, 0U);
}
