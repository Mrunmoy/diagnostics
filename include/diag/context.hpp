#pragma once

#include "diag/features.hpp"
#if DIAG_FEATURE_DTC
#include "diag/dtc.hpp"
#endif
#if DIAG_FEATURE_IDENTITY
#include "diag/identity.hpp"
#endif
#if DIAG_FEATURE_LIFECYCLE
#include "diag/lifecycle.hpp"
#endif
#include "diag/result.hpp"
#if DIAG_FEATURE_STORAGE
#include "diag/storage.hpp"
#endif

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
#if DIAG_FEATURE_DTC
    DtcRecord  *dtcRecords{nullptr};
    std::size_t dtcCapacity{0U};
#endif
};

class Context;

struct ContextStorage
{
#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
#if DIAG_FEATURE_DTC && DIAG_FEATURE_LIFECYCLE && DIAG_FEATURE_IDENTITY
    static constexpr std::size_t kSize = 176U;
#elif DIAG_FEATURE_DTC && DIAG_FEATURE_LIFECYCLE
    static constexpr std::size_t kSize = 160U;
#elif DIAG_FEATURE_DTC || DIAG_FEATURE_LIFECYCLE
    static constexpr std::size_t kSize = 144U;
#else
    static constexpr std::size_t kSize = 128U;
#endif
#else
#if DIAG_FEATURE_DTC && DIAG_FEATURE_LIFECYCLE && DIAG_FEATURE_IDENTITY
    static constexpr std::size_t kSize = 96U;
#elif DIAG_FEATURE_DTC && DIAG_FEATURE_LIFECYCLE
    static constexpr std::size_t kSize = 80U;
#elif DIAG_FEATURE_DTC || DIAG_FEATURE_LIFECYCLE
    static constexpr std::size_t kSize = 64U;
#elif DIAG_FEATURE_IDENTITY
    static constexpr std::size_t kSize = 48U;
#else
    static constexpr std::size_t kSize = 32U;
#endif
#endif
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

    [[nodiscard]] bool       isInitialized() const noexcept;
    [[nodiscard]] DirtyFlags dirtyFlags() const noexcept;
#if DIAG_FEATURE_IDENTITY
    [[nodiscard]] ResultValue<Identity> identity() const noexcept;
#endif
#if DIAG_FEATURE_DTC
    [[nodiscard]] std::size_t            dtcCount() const noexcept;
    [[nodiscard]] ResultValue<DtcRecord> dtc(DtcId id) const noexcept;
#endif
#if DIAG_FEATURE_LIFECYCLE
    [[nodiscard]] ResultValue<LifecycleSnapshot> lifecycle() const noexcept;
#endif

    [[nodiscard]] Result markDirty(DirtyFlag flag) noexcept;
    [[nodiscard]] Result clearDirty(DirtyFlag flag) noexcept;
#if DIAG_FEATURE_IDENTITY
    [[nodiscard]] Result attachIdentity(const Identity &identity) noexcept;
#endif
#if DIAG_FEATURE_LIFECYCLE
    [[nodiscard]] Result attachLifecycle(const LifecycleConfig &config) noexcept;
    [[nodiscard]] Result observeReset(ResetReason reason) noexcept;
    [[nodiscard]] Result clearLifecycleDirty(LifecycleDirtyFlags dirtyFlags) noexcept;
#endif
#if DIAG_FEATURE_DTC
    [[nodiscard]] Result registerDtc(DtcId id, DtcSeverity severity) noexcept;
    [[nodiscard]] Result listDtcs(DtcRecord *records, std::size_t capacity,
                                  std::size_t &count) const noexcept;
    [[nodiscard]] Result setDtcActive(DtcId id, bool active) noexcept;
    [[nodiscard]] Result clearDtc(DtcId id) noexcept;
#endif
#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
    [[nodiscard]] Result attachStorage(const Storage &storage) noexcept;
    [[nodiscard]] Result savePersistent() noexcept;
    [[nodiscard]] Result loadPersistent() noexcept;
    [[nodiscard]] Result clearPersistent() noexcept;
#endif

  private:
    struct State;
    struct StorageLayout;

#if DIAG_FEATURE_DTC && DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
    [[nodiscard]] static Result encodeDtcPayload(const State &state, std::uint8_t *payload,
                                                 std::size_t  capacity,
                                                 std::size_t &usedLength) noexcept;
    [[nodiscard]] static Result decodeDtcPayload(State &state, const std::uint8_t *payload,
                                                 std::size_t length) noexcept;
#endif
#if DIAG_FEATURE_LIFECYCLE && DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
    [[nodiscard]] static Result encodeLifecyclePayload(const State &state, std::uint8_t *payload,
                                                       std::size_t  capacity,
                                                       std::size_t &usedLength) noexcept;
    [[nodiscard]] static Result decodeLifecyclePayload(State &state, const std::uint8_t *payload,
                                                       std::size_t length) noexcept;
#endif

    StorageLayout *m_storage{nullptr};
    State         *m_state{nullptr};
};

} // namespace diag
