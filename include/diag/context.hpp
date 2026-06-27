#pragma once

#include "diag/dtc.hpp"
#include "diag/identity.hpp"
#include "diag/result.hpp"

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

struct Config
{
    DtcRecord  *dtcRecords{nullptr};
    std::size_t dtcCapacity{0U};
};

class Context;

struct ContextStorage
{
    static constexpr std::size_t kSize = 128U;
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

    [[nodiscard]] bool                   isInitialized() const noexcept;
    [[nodiscard]] DirtyFlags             dirtyFlags() const noexcept;
    [[nodiscard]] ResultValue<Identity>  identity() const noexcept;
    [[nodiscard]] std::size_t            dtcCount() const noexcept;
    [[nodiscard]] ResultValue<DtcRecord> dtc(DtcId id) const noexcept;

    [[nodiscard]] Result markDirty(DirtyFlag flag) noexcept;
    [[nodiscard]] Result clearDirty(DirtyFlag flag) noexcept;
    [[nodiscard]] Result attachIdentity(const Identity &identity) noexcept;
    [[nodiscard]] Result registerDtc(DtcId id, DtcSeverity severity) noexcept;
    [[nodiscard]] Result listDtcs(DtcRecord *records, std::size_t capacity,
                                  std::size_t &count) const noexcept;
    [[nodiscard]] Result setDtcActive(DtcId id, bool active) noexcept;
    [[nodiscard]] Result clearDtc(DtcId id) noexcept;

  private:
    struct State;
    struct StorageLayout;

    StorageLayout *m_storage{nullptr};
    State         *m_state{nullptr};
};

} // namespace diag
