#include <gtest/gtest.h>

#include <cstdint>

// The library under test is C99. Pull its public headers in with C linkage so
// the C++ test translation unit links against the unmangled symbols.
extern "C"
{
#include "diag/diag.h"
}

namespace
{

struct ContextFixture
{
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = nullptr;
};

struct diag_config make_valid_config(ContextFixture &fixture)
{
    struct diag_config config = {};
    (void)fixture;

    return config;
}

TEST(DiagContextInit, RejectsNullArguments)
{
    ContextFixture           fixture;
    const struct diag_config config = make_valid_config(fixture);

    EXPECT_EQ(diag_init(nullptr, &config, &fixture.ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&fixture.storage, nullptr, &fixture.ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_init(&fixture.storage, &config, nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextInit, AcceptsValidConfiguration)
{
    ContextFixture           fixture;
    const struct diag_config config = make_valid_config(fixture);

    EXPECT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    EXPECT_NE(fixture.ctx, nullptr);
    EXPECT_EQ(diag_deinit(fixture.ctx), DIAG_OK);
}

TEST(DiagContextLifetime, DeinitRejectsNullContext)
{
    EXPECT_EQ(diag_deinit(nullptr), DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagContextDirtyFlags, ReportsCleanInitialState)
{
    ContextFixture           fixture;
    const struct diag_config config = make_valid_config(fixture);
    uint32_t                 dirty_flags = UINT32_MAX;

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);

    EXPECT_EQ(diag_get_dirty_flags(fixture.ctx, &dirty_flags), DIAG_OK);
    EXPECT_EQ(dirty_flags, DIAG_DIRTY_NONE);
}

TEST(DiagContextDirtyFlags, RejectsInvalidOrUninitializedContext)
{
    ContextFixture           fixture;
    const struct diag_config config = make_valid_config(fixture);
    uint32_t                 dirty_flags = UINT32_MAX;

    EXPECT_EQ(diag_get_dirty_flags(nullptr, &dirty_flags), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_get_dirty_flags(fixture.ctx, nullptr), DIAG_ERROR_INVALID_ARGUMENT);

    ASSERT_EQ(diag_init(&fixture.storage, &config, &fixture.ctx), DIAG_OK);
    ASSERT_EQ(diag_deinit(fixture.ctx), DIAG_OK);

    EXPECT_EQ(diag_get_dirty_flags(fixture.ctx, &dirty_flags), DIAG_ERROR_NOT_INITIALIZED);
}

TEST(DiagContextStorage, IsAlignedForOpaqueContext)
{
    struct diag_context_storage storage = {};

    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(&storage.bytes[0]) % DIAG_CONTEXT_STORAGE_ALIGN, 0u);
    EXPECT_EQ(sizeof(storage), static_cast<std::size_t>(DIAG_CONTEXT_STORAGE_SIZE));
    EXPECT_EQ(sizeof(storage.bytes), static_cast<std::size_t>(DIAG_CONTEXT_STORAGE_SIZE));
}

TEST(DiagContextInit, RejectsMisalignedStorageWithoutDereferencingIt)
{
    // clang-format off
    alignas(DIAG_CONTEXT_STORAGE_ALIGN)
        uint8_t raw[sizeof(struct diag_context_storage) + DIAG_CONTEXT_STORAGE_ALIGN] = {};
    // clang-format on
    ContextFixture           fixture;
    struct diag_context     *ctx = reinterpret_cast<struct diag_context *>(0xDEADBEEF);
    const struct diag_config config = make_valid_config(fixture);

    auto *misaligned_storage = reinterpret_cast<struct diag_context_storage *>(&raw[1]);

    ASSERT_NE(reinterpret_cast<std::uintptr_t>(misaligned_storage) % DIAG_CONTEXT_STORAGE_ALIGN,
              0u);
    EXPECT_EQ(diag_init(misaligned_storage, &config, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(ctx, nullptr);
}

TEST(DiagContextInit, ClearsOutContextOnFailure)
{
    // A failed init must leave the out-parameter in a known state so a stale
    // pointer from a previous successful init cannot be reused by accident.
    struct diag_context_storage storage = {};
    struct diag_context        *ctx = reinterpret_cast<struct diag_context *>(0xDEADBEEF);

    EXPECT_EQ(diag_init(&storage, nullptr, &ctx), DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(ctx, nullptr);
}

} // namespace
