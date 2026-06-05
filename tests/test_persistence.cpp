#include <array>
#include <cstddef>
#include <cstdint>

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
    unsigned int load_calls;
    unsigned int save_calls;
    unsigned int clear_calls;
};

extern "C" enum diag_result persistence_fake_load(void *user, uint8_t *buffer, size_t buffer_size,
                                                  size_t *bytes_read)
{
    auto *fake = static_cast<FakeStorage *>(user);

    (void)buffer;
    (void)buffer_size;
    fake->load_calls++;
    *bytes_read = 0u;

    return DIAG_OK;
}

extern "C" enum diag_result persistence_fake_save(void *user, const uint8_t *buffer, size_t size)
{
    auto *fake = static_cast<FakeStorage *>(user);

    (void)buffer;
    (void)size;
    fake->save_calls++;

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
    };

    return storage;
}

/** Combines DTC and storage modules to test the explicit persistence boundary. */
struct PersistenceFixture : public testing::Test
{
    struct diag_context_storage             context_storage = {};
    std::array<struct diag_dtc_snapshot, 2> dtc_buffer = {};
    FakeStorage                             fake = {};
    struct diag_context                    *ctx = nullptr;
    struct diag_storage                     storage = make_storage(&fake);

    void SetUp() override
    {
        const struct diag_config config = {};
        struct diag_dtc_config   dtc_config = {};

        dtc_config.records = dtc_buffer.data();
        dtc_config.capacity = dtc_buffer.size();

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

TEST_F(PersistenceFixture, SaveDirtyContextFailsUntilCapsuleSerializerExists)
{
    uint32_t dirty_flags = DIAG_DIRTY_NONE;

    ASSERT_EQ(diag_dtc_register(ctx, 0x2202u, DIAG_DTC_SEVERITY_ERROR), DIAG_OK);
    ASSERT_EQ(diag_get_dirty_flags(ctx, &dirty_flags), DIAG_OK);
    ASSERT_EQ(dirty_flags, DIAG_DIRTY_DTC);

    EXPECT_EQ(diag_save(ctx), DIAG_ERROR_NOT_SUPPORTED);
    EXPECT_EQ(fake.save_calls, 0u);
}

} // namespace
