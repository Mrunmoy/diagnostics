#pragma once

#include "diag/dtc.hpp"
#include "diag/features.hpp"
#include "diag/identity.hpp"
#include "diag/lifecycle.hpp"
#include "diag/result.hpp"
#include "diag/storage.hpp"

#include <cstddef>
#include <cstdint>

namespace diag
{

using DirtyFlags = std::uint32_t;

enum class DirtyFlag : DirtyFlags
{
    None = 0U,
    Dtc = 1U << 0U,
    Lifecycle = 1U << 1U,
};

constexpr DirtyFlags operator|(const DirtyFlag lhs, const DirtyFlag rhs)
{
    return static_cast<DirtyFlags>(lhs) | static_cast<DirtyFlags>(rhs);
}

namespace internal
{

struct EmptyFeature
{
};

template <bool Enabled> struct ConfigDtcFields : EmptyFeature
{
};

template <> struct ConfigDtcFields<true>
{
    DtcRecord  *dtcRecords{nullptr};
    std::size_t dtcCapacity{0U};
};

[[nodiscard]] constexpr std::size_t contextStorageSize() noexcept
{
    std::size_t size = 32U;

    if (features::kDtc || features::kLifecycle)
    {
        size = 64U;
    }

    if (features::kDtc && features::kLifecycle)
    {
        size = 80U;
    }

    if (features::kIdentity)
    {
        size += 16U;
    }

    if (features::kPersistence)
    {
        size += 80U;
    }

    return size;
}

} // namespace internal

struct Config : internal::ConfigDtcFields<features::kDtc>
{
};

class Context;

struct ContextStorage
{
    static constexpr std::size_t kSize = internal::contextStorageSize();
    static constexpr std::size_t kAlignment = alignof(std::max_align_t);
    static_assert((kSize % kAlignment) == 0U,
                  "ContextStorage::kSize must be a multiple of kAlignment");

    alignas(kAlignment) std::uint8_t bytes[kSize]{};
};

class Context
{
  public:
    explicit Context(ContextStorage &storage, const Config &config = Config{}) noexcept;
    ~Context() noexcept;

    Context(const Context &) = delete;
    Context &operator=(const Context &) = delete;
    Context(Context &&) = delete;
    Context &operator=(Context &&) = delete;

    [[nodiscard]] bool                           isInitialized() const noexcept;
    [[nodiscard]] DirtyFlags                     dirtyFlags() const noexcept;
    [[nodiscard]] ResultValue<Identity>          identity() const noexcept;
    [[nodiscard]] std::size_t                    dtcCount() const noexcept;
    [[nodiscard]] ResultValue<DtcRecord>         dtc(DtcId id) const noexcept;
    [[nodiscard]] ResultValue<LifecycleSnapshot> lifecycle() const noexcept;
    [[nodiscard]] Result                         markDirty(DirtyFlag flag) noexcept;
    [[nodiscard]] Result                         clearDirty(DirtyFlag flag) noexcept;
    [[nodiscard]] Result                         attachIdentity(const Identity &identity) noexcept;
    [[nodiscard]] Result attachLifecycle(const LifecycleConfig &config) noexcept;
    [[nodiscard]] Result observeReset(ResetReason reason) noexcept;
    [[nodiscard]] Result clearLifecycleDirty(LifecycleDirtyFlags dirtyFlags) noexcept;
    [[nodiscard]] Result registerDtc(DtcId id, DtcSeverity severity) noexcept;
    [[nodiscard]] Result listDtcs(DtcRecord *records, std::size_t capacity,
                                  std::size_t &count) const noexcept;
    [[nodiscard]] Result setDtcActive(DtcId id, bool active) noexcept;
    [[nodiscard]] Result clearDtc(DtcId id) noexcept;
    [[nodiscard]] Result attachStorage(const Storage &storage) noexcept;
    [[nodiscard]] Result savePersistent() noexcept;
    [[nodiscard]] Result loadPersistent() noexcept;
    [[nodiscard]] Result clearPersistent() noexcept;

  private:
    struct State;
    struct StorageLayout;

    StorageLayout *m_storage{nullptr};
    State         *m_state{nullptr};
};

} // namespace diag
