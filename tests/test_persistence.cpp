#include <array>
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

/** Counts storage callbacks so DTC tests can prove runtime mutations stay RAM-only. */
struct FakeStorage
{
    std::array<uint8_t, 256> bytes = {};
    std::size_t              used = 0u;
    unsigned int             load_calls;
    unsigned int             save_calls;
    unsigned int             clear_calls;
};

extern "C" enum diag_result persistence_fake_load(void *user, uint8_t *buffer, size_t buffer_size,
                                                  size_t *bytes_read)
{
    auto *fake = static_cast<FakeStorage *>(user);

    fake->load_calls++;
    if (buffer_size < fake->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    std::memcpy(buffer, fake->bytes.data(), fake->used);
    *bytes_read = fake->used;

    return DIAG_OK;
}

extern "C" enum diag_result persistence_fake_save(void *user, const uint8_t *buffer, size_t size)
{
    auto *fake = static_cast<FakeStorage *>(user);

    fake->save_calls++;
    if (size > fake->bytes.size())
    {
        return DIAG_ERROR_CAPACITY;
    }

    std::memcpy(fake->bytes.data(), buffer, size);
    fake->used = size;

    return DIAG_OK;
}

extern "C" enum diag_result persistence_fake_clear(void *user)
{
    auto *fake = static_cast<FakeStorage *>(user);

    fake->clear_calls++;

    return DIAG_OK;
}

const struct diag_storage_ops kFakeOps = {
    /* load  */ persistence_fake_load,
    /* save  */ persistence_fake_save,
    /* clear */ persistence_fake_clear,
};

struct diag_storage make_storage(FakeStorage *fake)
{
    const struct diag_storage storage = {
        /* ops          */ &kFakeOps,
        /* user         */ fake,
        /* capabilities */
        {
            /* erase_value     */ 0xFFu,
            /* write_alignment */ 4u,
            /* atomic_commit   */ DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
            /* wear_leveling   */ DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
        },
        /* capsule_buffer      */ nullptr,
        /* capsule_buffer_size */ 0u,
    };

    return storage;
}

/** Combines DTC and storage modules to test the explicit persistence boundary. */
struct PersistenceFixture : public testing::Test
{
    struct diag_context_storage             context_storage = {};
    std::array<struct diag_dtc_snapshot, 2> dtc_buffer = {};
    std::array<uint8_t, 256>                capsule_buffer = {};
    FakeStorage                             fake = {};
    struct diag_context                    *ctx = nullptr;
    struct diag_storage                     storage = make_storage(&fake);

    void SetUp() override
    {
        const struct diag_config config = {};
        struct diag_dtc_config   dtc_config = {};

        dtc_config.records = dtc_buffer.data();
        dtc_config.capacity = dtc_buffer.size();
        storage.capsule_buffer = capsule_buffer.data();
        storage.capsule_buffer_size = capsule_buffer.size();

        ASSERT_EQ(diag_init(&context_storage, &config, &ctx), DIAG_OK);
        ASSERT_EQ(diag_storage_attach(ctx, &storage), DIAG_OK);
        ASSERT_EQ(diag_dtc_attach(ctx, &dtc_config), DIAG_OK);
    }
};

TEST_F(PersistenceFixture, DtcMutationMarksDirtyWithoutCallingStorage)
{
    uint32_t dirty_flags = DIAG_DIRTY_NONE;

    ASSERT_EQ(diag_dtc_register(ctx, 0x2201u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_dtc_set_active(ctx, 0x2201u), DIAG_OK);
    ASSERT_EQ(diag_get_dirty_flags(ctx, &dirty_flags), DIAG_OK);

    EXPECT_EQ(dirty_flags, DIAG_DIRTY_DTC);
    EXPECT_EQ(fake.load_calls, 0u);
    EXPECT_EQ(fake.save_calls, 0u);
    EXPECT_EQ(fake.clear_calls, 0u);
}

TEST_F(PersistenceFixture, SaveDirtyDtcWritesCapsuleAndClearsDirtyFlag)
{
    uint32_t dirty_flags = DIAG_DIRTY_NONE;

    ASSERT_EQ(diag_dtc_register(ctx, 0x2202u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_get_dirty_flags(ctx, &dirty_flags), DIAG_OK);
    ASSERT_EQ(dirty_flags, DIAG_DIRTY_DTC);

    ASSERT_EQ(diag_save(ctx), DIAG_OK);
    ASSERT_EQ(diag_get_dirty_flags(ctx, &dirty_flags), DIAG_OK);

    EXPECT_EQ(dirty_flags, DIAG_DIRTY_NONE);
    EXPECT_EQ(fake.save_calls, 1u);
    EXPECT_GT(fake.used, 0u);
    EXPECT_EQ(fake.bytes[0], 'D');
    EXPECT_EQ(fake.bytes[1], 'G');
    EXPECT_EQ(fake.bytes[2], 'C');
    EXPECT_EQ(fake.bytes[3], 'P');
}

TEST_F(PersistenceFixture, SaveDirtyDtcRequiresCallerOwnedCapsuleBuffer)
{
    struct diag_context_storage             context_storage = {};
    std::array<struct diag_dtc_snapshot, 1> buffer = {};
    FakeStorage                             local_fake = {};
    struct diag_storage                     local_storage = make_storage(&local_fake);
    struct diag_context                    *local_ctx = nullptr;
    struct diag_dtc_config                  dtc_config = {};
    const struct diag_config                config = {};

    dtc_config.records = buffer.data();
    dtc_config.capacity = buffer.size();

    ASSERT_EQ(diag_init(&context_storage, &config, &local_ctx), DIAG_OK);
    ASSERT_EQ(diag_storage_attach(local_ctx, &local_storage), DIAG_OK);
    ASSERT_EQ(diag_dtc_attach(local_ctx, &dtc_config), DIAG_OK);
    ASSERT_EQ(diag_dtc_register(local_ctx, 0x2205u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);

    EXPECT_EQ(diag_save(local_ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(local_fake.save_calls, 0u);
}

TEST_F(PersistenceFixture, LoadSavedDtcCapsuleRestoresRegisteredRecords)
{
    ASSERT_EQ(diag_dtc_register_fault(ctx, 0x1001u, 0x0A2203u, DIAG_DTC_SEVERITY_CRITICAL),
              DIAG_OK);
    ASSERT_EQ(diag_dtc_set_fault_test_failed(ctx, 0x1001u), DIAG_OK);
    ASSERT_EQ(diag_dtc_operation_cycle(ctx), DIAG_OK);
    ASSERT_EQ(diag_save(ctx), DIAG_OK);

    struct diag_context_storage             restored_context_storage = {};
    std::array<struct diag_dtc_snapshot, 2> restored_dtc_buffer = {};
    std::array<uint8_t, 256>                restored_capsule_buffer = {};
    struct diag_context                    *restored_ctx = nullptr;
    struct diag_storage                     restored_storage = make_storage(&fake);
    struct diag_dtc_config                  restored_dtc_config = {};
    const struct diag_config                config = {};

    restored_storage.capsule_buffer = restored_capsule_buffer.data();
    restored_storage.capsule_buffer_size = restored_capsule_buffer.size();
    restored_dtc_config.records = restored_dtc_buffer.data();
    restored_dtc_config.capacity = restored_dtc_buffer.size();

    ASSERT_EQ(diag_init(&restored_context_storage, &config, &restored_ctx), DIAG_OK);
    ASSERT_EQ(diag_storage_attach(restored_ctx, &restored_storage), DIAG_OK);
    ASSERT_EQ(diag_dtc_attach(restored_ctx, &restored_dtc_config), DIAG_OK);

    ASSERT_EQ(diag_load(restored_ctx), DIAG_OK);

    struct diag_dtc_snapshot snapshot = {};
    ASSERT_EQ(diag_dtc_get(restored_ctx, 0x0A2203u, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.local_fault_id, 0x1001u);
    EXPECT_EQ(snapshot.severity, DIAG_DTC_SEVERITY_CRITICAL);
    EXPECT_TRUE(snapshot.active);
    EXPECT_TRUE(snapshot.status & DIAG_DTC_STATUS_CONFIRMED);
    EXPECT_EQ(snapshot.occurrence_count, 1u);
    EXPECT_EQ(snapshot.active_count, 1u);
}

TEST_F(PersistenceFixture, LoadRejectsCorruptDtcPayloadAfterCapsuleCrcPasses)
{
    ASSERT_EQ(diag_dtc_register(ctx, 0x2204u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_save(ctx), DIAG_OK);

    struct diag_capsule_descriptor     descriptor = {};
    const struct diag_capsule_section *section = nullptr;
    std::size_t                        encoded_length = 0u;

    ASSERT_EQ(diag_capsule_decode(fake.bytes.data(), fake.used, &descriptor), DIAG_OK);
    ASSERT_EQ(diag_capsule_find_section_by_type(&descriptor, DIAG_CAPSULE_SECTION_APPLICATION_DTC,
                                                &section),
              DIAG_OK);
    ASSERT_NE(section, nullptr);

    constexpr std::size_t kReservedByteOffsetInFirstRecord = 4u + 13u;
    fake.bytes[section->offset + kReservedByteOffsetInFirstRecord] = 0xA5u;
    ASSERT_EQ(
        diag_capsule_encode_v1(fake.bytes.data(), fake.bytes.size(), &descriptor, &encoded_length),
        DIAG_OK);
    fake.used = encoded_length;

    EXPECT_EQ(diag_load(ctx), DIAG_ERROR_CORRUPT_DATA);
}

} // namespace
