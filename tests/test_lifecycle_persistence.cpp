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

struct FakeStorage
{
    std::array<uint8_t, 128> bytes = {};
    std::size_t              used = 0u;
    unsigned int             load_calls;
    unsigned int             save_calls;
    unsigned int             clear_calls;
};

extern "C" enum diag_result lifecycle_persistence_fake_load(void *user, uint8_t *buffer,
                                                            size_t buffer_size, size_t *bytes_read)
{
    auto *fake = static_cast<FakeStorage *>(user);

    ++fake->load_calls;
    if (buffer_size < fake->used)
    {
        return DIAG_ERROR_CAPACITY;
    }

    std::memcpy(buffer, fake->bytes.data(), fake->used);
    *bytes_read = fake->used;

    return DIAG_OK;
}

extern "C" enum diag_result lifecycle_persistence_fake_save(void *user, const uint8_t *buffer,
                                                            size_t size)
{
    auto *fake = static_cast<FakeStorage *>(user);

    ++fake->save_calls;
    if (size > fake->bytes.size())
    {
        return DIAG_ERROR_CAPACITY;
    }

    std::memcpy(fake->bytes.data(), buffer, size);
    fake->used = size;

    return DIAG_OK;
}

extern "C" enum diag_result lifecycle_persistence_fake_clear(void *user)
{
    auto *fake = static_cast<FakeStorage *>(user);

    ++fake->clear_calls;

    return DIAG_OK;
}

const struct diag_storage_ops kFakeOps = {
    /* load  */ lifecycle_persistence_fake_load,
    /* save  */ lifecycle_persistence_fake_save,
    /* clear */ lifecycle_persistence_fake_clear,
};

struct diag_storage make_storage(FakeStorage *fake, uint8_t *capsule_buffer,
                                 std::size_t capsule_buffer_size, uint32_t write_alignment)
{
    const struct diag_storage storage = {
        /* ops          */ &kFakeOps,
        /* user         */ fake,
        /* capabilities */
        {
            /* erase_value     */ 0xFFu,
            /* write_alignment */ write_alignment,
            /* atomic_commit   */ DIAG_STORAGE_ATOMIC_COMMIT_ADAPTER,
            /* wear_leveling   */ DIAG_STORAGE_WEAR_LEVELING_ADAPTER,
        },
        /* capsule_buffer      */ capsule_buffer,
        /* capsule_buffer_size */ capsule_buffer_size,
    };

    return storage;
}

struct diag_storage make_storage(FakeStorage *fake, uint8_t *capsule_buffer,
                                 std::size_t capsule_buffer_size)
{
    return make_storage(fake, capsule_buffer, capsule_buffer_size, 4u);
}

struct LifecyclePersistenceFixture : public testing::Test
{
    struct diag_context_storage context_storage = {};
    std::array<uint8_t, 128>    capsule_buffer = {};
    FakeStorage                 fake = {};
    struct diag_context        *ctx = nullptr;
    struct diag_storage storage = make_storage(&fake, capsule_buffer.data(), capsule_buffer.size());

    void SetUp() override
    {
        const struct diag_config     config = {};
        struct diag_lifecycle_config lifecycle_config = {};

        lifecycle_config.reset_counter_policy = DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY;

        ASSERT_EQ(diag_init(&context_storage, &config, &ctx), DIAG_OK);
        ASSERT_EQ(diag_storage_attach(ctx, &storage), DIAG_OK);
        ASSERT_EQ(diag_lifecycle_attach(ctx, &lifecycle_config), DIAG_OK);
    }
};

TEST_F(LifecyclePersistenceFixture, SaveAndLoadLifecycleWithoutDtcFeatureDependency)
{
    ASSERT_EQ(diag_lifecycle_observe_reset(ctx, DIAG_RESET_REASON_FAULT), DIAG_OK);
    ASSERT_EQ(diag_save(ctx), DIAG_OK);

    struct diag_context_storage restored_context_storage = {};
    std::array<uint8_t, 128>    restored_capsule_buffer = {};
    struct diag_context        *restored_ctx = nullptr;
    struct diag_storage         restored_storage =
        make_storage(&fake, restored_capsule_buffer.data(), restored_capsule_buffer.size());
    struct diag_lifecycle_config restored_lifecycle_config = {};
    const struct diag_config     config = {};

    restored_lifecycle_config.reset_counter_policy = DIAG_RESET_COUNTER_POLICY_RAM_ONLY;

    ASSERT_EQ(diag_init(&restored_context_storage, &config, &restored_ctx), DIAG_OK);
    ASSERT_EQ(diag_storage_attach(restored_ctx, &restored_storage), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_attach(restored_ctx, &restored_lifecycle_config), DIAG_OK);
    ASSERT_EQ(diag_load(restored_ctx), DIAG_OK);

    struct diag_lifecycle_snapshot snapshot = {};
    ASSERT_EQ(diag_lifecycle_get(restored_ctx, &snapshot), DIAG_OK);
    EXPECT_EQ(snapshot.last_reset_reason, DIAG_RESET_REASON_FAULT);
    EXPECT_EQ(snapshot.reset_counter_policy, DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY);
    EXPECT_EQ(snapshot.reset_count, 1u);
    EXPECT_EQ(snapshot.abnormal_reset_count, 1u);
    EXPECT_EQ(snapshot.dirty_flags, DIAG_LIFECYCLE_DIRTY_NONE);
    EXPECT_EQ(fake.save_calls, 1u);
}

TEST_F(LifecyclePersistenceFixture, SavePadsCapsuleToAdapterWriteAlignment)
{
    struct diag_context_storage aligned_context_storage = {};
    std::array<uint8_t, 128>    aligned_capsule_buffer = {};
    FakeStorage                 aligned_fake = {};
    struct diag_context        *aligned_ctx = nullptr;
    struct diag_storage aligned_storage = make_storage(&aligned_fake, aligned_capsule_buffer.data(),
                                                       aligned_capsule_buffer.size(), 16u);
    const struct diag_config     config = {};
    struct diag_lifecycle_config lifecycle_config = {};

    lifecycle_config.reset_counter_policy = DIAG_RESET_COUNTER_POLICY_ABNORMAL_ONLY;

    ASSERT_EQ(diag_init(&aligned_context_storage, &config, &aligned_ctx), DIAG_OK);
    ASSERT_EQ(diag_storage_attach(aligned_ctx, &aligned_storage), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_attach(aligned_ctx, &lifecycle_config), DIAG_OK);
    ASSERT_EQ(diag_lifecycle_observe_reset(aligned_ctx, DIAG_RESET_REASON_WATCHDOG), DIAG_OK);

    ASSERT_EQ(diag_save(aligned_ctx), DIAG_OK);

    struct diag_capsule_descriptor descriptor = {};
    ASSERT_EQ(diag_capsule_decode(aligned_fake.bytes.data(), aligned_fake.used, &descriptor),
              DIAG_OK);
    EXPECT_EQ(aligned_fake.used % 16u, 0u);
    EXPECT_EQ(aligned_fake.used, descriptor.total_length);
}

} // namespace
