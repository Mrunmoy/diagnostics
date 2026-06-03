#include <cstddef>
#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>

extern "C"
{
#include "diag/diag.h"
}

namespace
{

struct FakeStorage
{
    uint8_t      bytes[32];
    size_t       used;
    unsigned int load_calls;
    unsigned int save_calls;
    unsigned int clear_calls;
    unsigned int fake_flash_writes;
    unsigned int fake_flash_erases;
};

extern "C" enum diag_result fake_load(void *user, uint8_t *buffer, size_t buffer_size,
                                      size_t *bytes_read)
{
    auto *fake = static_cast<FakeStorage *>(user);

    fake->load_calls++;

    if (buffer_size < fake->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    std::memcpy(buffer, fake->bytes, fake->used);
    *bytes_read = fake->used;
    return DIAG_OK;
}

extern "C" enum diag_result fake_save(void *user, const uint8_t *buffer, size_t size)
{
    auto *fake = static_cast<FakeStorage *>(user);

    fake->save_calls++;

    if (size > sizeof(fake->bytes))
    {
        return DIAG_ERROR_CAPACITY;
    }

    std::memcpy(fake->bytes, buffer, size);
    fake->used = size;
    fake->fake_flash_writes++;
    return DIAG_OK;
}

extern "C" enum diag_result fake_clear(void *user)
{
    auto *fake = static_cast<FakeStorage *>(user);

    fake->clear_calls++;
    std::memset(fake->bytes, 0xFF, sizeof(fake->bytes));
    fake->used = 0u;
    fake->fake_flash_erases++;
    return DIAG_OK;
}

const struct diag_storage_ops kFakeOps = {
    /* load  */ fake_load,
    /* save  */ fake_save,
    /* clear */ fake_clear,
};

struct diag_storage make_storage(FakeStorage *fake)
{
    const struct diag_storage storage = {
        /* ops          */ &kFakeOps,
        /* user         */ fake,
        /* capabilities */
        {
            /* erase_value   */ 0xFFu,
            /* write_alignment */ 4u,
            /* atomic_commit */ DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
            /* wear_leveling */ DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
        },
    };

    return storage;
}

TEST(DiagStorageCapabilities, RejectsInvalidArguments)
{
    const struct diag_storage_capabilities zero_alignment = {
        /* erase_value   */ 0xFFu,
        /* write_alignment */ 0u,
        /* atomic_commit */ DIAG_STORAGE_ATOMIC_COMMIT_NONE,
        /* wear_leveling */ DIAG_STORAGE_WEAR_LEVELING_NONE,
    };

    const struct diag_storage_capabilities invalid_atomic = {
        /* erase_value   */ 0xFFu,
        /* write_alignment */ 1u,
        /* atomic_commit */ static_cast<enum diag_storage_atomic_commit>(99),
        /* wear_leveling */ DIAG_STORAGE_WEAR_LEVELING_NONE,
    };

    const struct diag_storage_capabilities invalid_wear = {
        /* erase_value   */ 0xFFu,
        /* write_alignment */ 1u,
        /* atomic_commit */ DIAG_STORAGE_ATOMIC_COMMIT_NONE,
        /* wear_leveling */ static_cast<enum diag_storage_wear_leveling>(99),
    };

    EXPECT_EQ(diag_storage_validate_capabilities(nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_validate_capabilities(&zero_alignment), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_validate_capabilities(&invalid_atomic), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_validate_capabilities(&invalid_wear), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagStorageCapabilities, AcceptsValidCapabilityCombinations)
{
    const struct diag_storage_capabilities minimal = {
        /* erase_value   */ 0x00u,
        /* write_alignment */ 1u,
        /* atomic_commit */ DIAG_STORAGE_ATOMIC_COMMIT_NONE,
        /* wear_leveling */ DIAG_STORAGE_WEAR_LEVELING_NONE,
    };

    const struct diag_storage_capabilities adapter_managed = {
        /* erase_value   */ 0xFFu,
        /* write_alignment */ 8u,
        /* atomic_commit */ DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
        /* wear_leveling */ DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
    };

    EXPECT_EQ(diag_storage_validate_capabilities(&minimal), DIAG_OK);
    EXPECT_EQ(diag_storage_validate_capabilities(&adapter_managed), DIAG_OK);
}

TEST(DiagStorageValidate, RejectsMissingRequiredCallbacks)
{
    FakeStorage                   fake = {};
    const struct diag_storage_ops missing_save = {
        /* load  */ fake_load,
        /* save  */ nullptr,
        /* clear */ fake_clear,
    };
    const struct diag_storage storage = {
        /* ops          */ &missing_save,
        /* user         */ &fake,
        /* capabilities */
        {
            /* erase_value   */ 0xFFu,
            /* write_alignment */ 1u,
            /* atomic_commit */ DIAG_STORAGE_ATOMIC_COMMIT_NONE,
            /* wear_leveling */ DIAG_STORAGE_WEAR_LEVELING_NONE,
        },
    };

    EXPECT_EQ(diag_storage_validate(nullptr), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_validate(&storage), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagStorageOps, SaveLoadAndClearCallAdapterExactlyOnce)
{
    FakeStorage         fake = {};
    struct diag_storage storage = make_storage(&fake);
    const uint8_t       payload[8] = {0x10u, 0x11u, 0x12u, 0x13u, 0x14u, 0x15u, 0x16u, 0x17u};
    uint8_t             loaded[8] = {};
    size_t              loaded_size = 99u;

    EXPECT_EQ(diag_storage_save(&storage, payload, sizeof(payload)), DIAG_OK);
    EXPECT_EQ(fake.save_calls, 1u);
    EXPECT_EQ(fake.fake_flash_writes, 1u);

    EXPECT_EQ(diag_storage_load(&storage, loaded, sizeof(loaded), &loaded_size), DIAG_OK);
    EXPECT_EQ(fake.load_calls, 1u);
    EXPECT_EQ(loaded_size, sizeof(payload));
    EXPECT_EQ(std::memcmp(payload, loaded, sizeof(payload)), 0);

    EXPECT_EQ(diag_storage_clear(&storage), DIAG_OK);
    EXPECT_EQ(fake.clear_calls, 1u);
    EXPECT_EQ(fake.fake_flash_erases, 1u);
}

TEST(DiagStorageOps, RejectsInvalidArgumentsBeforeCallingAdapter)
{
    FakeStorage         fake = {};
    struct diag_storage storage = make_storage(&fake);
    const uint8_t       payload[8] = {};
    uint8_t             loaded[8] = {};
    size_t              loaded_size = 0u;

    EXPECT_EQ(diag_storage_save(&storage, nullptr, sizeof(payload)), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_save(&storage, payload, 0u), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_save(&storage, payload, 3u), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(fake.save_calls, 0u);

    EXPECT_EQ(diag_storage_load(&storage, nullptr, sizeof(loaded), &loaded_size),
              DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_load(&storage, loaded, 0u, &loaded_size), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_storage_load(&storage, loaded, sizeof(loaded), nullptr),
              DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(fake.load_calls, 0u);
}

TEST(DiagStorageOps, LoadClearsBytesReadBeforeAdapterFailure)
{
    FakeStorage fake = {};
    fake.used = 16u;
    struct diag_storage storage = make_storage(&fake);
    uint8_t             loaded[8] = {};
    size_t              loaded_size = 99u;

    EXPECT_EQ(diag_storage_load(&storage, loaded, sizeof(loaded), &loaded_size),
              DIAG_ERROR_CAPACITY);
    EXPECT_EQ(fake.load_calls, 1u);
    EXPECT_EQ(loaded_size, 0u);
}

} // namespace
