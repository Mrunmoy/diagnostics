#include "diag/context.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>

namespace diag
{

struct Context::State
{
    Config            config{};
    DirtyFlags        dirtyFlags{0U};
    std::size_t       dtcCount{0U};
    Identity          identity{};
    LifecycleConfig   lifecycleConfig{};
    LifecycleSnapshot lifecycleSnapshot{};
    bool              initialized{false};
    bool              identityAttached{false};
    bool              lifecycleAttached{false};
};

struct Context::StorageLayout
{
    bool engaged{false};
    alignas(State) std::uint8_t state[sizeof(State)];
};

Context::Context(ContextStorage &storage, const Config &config) noexcept
{
    static_assert(sizeof(StorageLayout) <= ContextStorage::kSize,
                  "ContextStorage is too small for Context::StorageLayout");
    static_assert(alignof(StorageLayout) <= ContextStorage::kAlignment,
                  "ContextStorage alignment is too small for Context::StorageLayout");

    if (storage.bytes[0] != 0U)
    {
        return;
    }

    m_storage = new (static_cast<void *>(storage.bytes)) StorageLayout{};
    m_storage->engaged = true;

    void *const rawStorage = static_cast<void *>(m_storage->state);
    m_state = new (rawStorage) State{};
    m_state->config = config;
    m_state->initialized = true;
}

Context::~Context() noexcept
{
    if (m_state != nullptr && m_storage != nullptr)
    {
        m_state->~State();
        m_storage->engaged = false;
        m_storage = nullptr;
        m_state = nullptr;
    }
}

bool Context::isInitialized() const noexcept
{
    return (m_state != nullptr) && m_state->initialized;
}

DirtyFlags Context::dirtyFlags() const noexcept
{
    return isInitialized() ? m_state->dirtyFlags : 0U;
}

namespace
{

template <typename Record>
[[nodiscard]] Record *findDtcRecord(Record *records, const std::size_t count,
                                    const DtcId id) noexcept
{
    for (std::size_t index = 0U; index < count; ++index)
    {
        if (records[index].id == id)
        {
            return &records[index];
        }
    }

    return nullptr;
}

} // namespace

ResultValue<Identity> Context::identity() const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<Identity>{Result::NotInitialized};
    }

    if (!m_state->identityAttached)
    {
        return ResultValue<Identity>{Result::NotFound};
    }

    return ResultValue<Identity>{m_state->identity};
}

ResultValue<LifecycleSnapshot> Context::lifecycle() const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<LifecycleSnapshot>{Result::NotInitialized};
    }

    if (!m_state->lifecycleAttached)
    {
        return ResultValue<LifecycleSnapshot>{Result::NotInitialized};
    }

    return ResultValue<LifecycleSnapshot>{m_state->lifecycleSnapshot};
}

std::size_t Context::dtcCount() const noexcept
{
    return isInitialized() ? m_state->dtcCount : 0U;
}

ResultValue<DtcRecord> Context::dtc(const DtcId id) const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<DtcRecord>{Result::NotInitialized};
    }

    const DtcRecord *const record =
        findDtcRecord(m_state->config.dtcRecords, m_state->dtcCount, id);
    if (record == nullptr)
    {
        return ResultValue<DtcRecord>{Result::NotFound};
    }

    return ResultValue<DtcRecord>{*record};
}

Result Context::markDirty(const DirtyFlag flag) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->dirtyFlags |= static_cast<DirtyFlags>(flag);
    return Result::Ok;
}

Result Context::clearDirty(const DirtyFlag flag) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->dirtyFlags &= ~static_cast<DirtyFlags>(flag);
    return Result::Ok;
}

Result Context::attachIdentity(const Identity &identity) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->identity = identity;
    m_state->identityAttached = true;

    return Result::Ok;
}

Result Context::attachLifecycle(const LifecycleConfig &config) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->lifecycleConfig = config;
    m_state->lifecycleSnapshot = LifecycleSnapshot{};
    m_state->lifecycleSnapshot.resetCounterPolicy = config.resetCounterPolicy;
    if (config.resetCounterPolicy == ResetCounterPolicy::Platform)
    {
        m_state->lifecycleSnapshot.resetCount = config.platformResetCount;
    }

    m_state->lifecycleAttached = true;
    return clearDirty(DirtyFlag::Lifecycle);
}

Result Context::observeReset(const ResetReason reason) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (!m_state->lifecycleAttached)
    {
        return Result::NotInitialized;
    }

    LifecycleSnapshot       &snapshot = m_state->lifecycleSnapshot;
    const ResetCounterPolicy policy = m_state->lifecycleConfig.resetCounterPolicy;
    const bool               abnormal = isAbnormalReset(reason);

    snapshot.lastResetReason = reason;

    if (policy != ResetCounterPolicy::Disabled && policy != ResetCounterPolicy::Platform)
    {
        if (snapshot.resetCount < std::numeric_limits<std::uint32_t>::max())
        {
            ++snapshot.resetCount;
        }

        if (abnormal && snapshot.abnormalResetCount < std::numeric_limits<std::uint32_t>::max())
        {
            ++snapshot.abnormalResetCount;
        }
    }

    switch (policy)
    {
    case ResetCounterPolicy::AbnormalOnly:
        if (abnormal)
        {
            snapshot.dirtyFlags |=
                static_cast<LifecycleDirtyFlags>(LifecycleDirtyFlag::ResetCounter);
            snapshot.persistRequested = true;
            return markDirty(DirtyFlag::Lifecycle);
        }
        break;

    case ResetCounterPolicy::EveryN:
        if (m_state->lifecycleConfig.resetCountInterval != 0U &&
            (snapshot.resetCount % m_state->lifecycleConfig.resetCountInterval) == 0U)
        {
            snapshot.dirtyFlags |=
                static_cast<LifecycleDirtyFlags>(LifecycleDirtyFlag::ResetCounter);
            snapshot.persistRequested = true;
            return markDirty(DirtyFlag::Lifecycle);
        }
        break;

    case ResetCounterPolicy::Disabled:
    case ResetCounterPolicy::RamOnly:
    case ResetCounterPolicy::Platform:
    default:
        break;
    }

    return Result::Ok;
}

Result Context::clearLifecycleDirty(const LifecycleDirtyFlags dirtyFlags) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (!m_state->lifecycleAttached)
    {
        return Result::NotInitialized;
    }

    m_state->lifecycleSnapshot.dirtyFlags &= ~dirtyFlags;
    m_state->lifecycleSnapshot.persistRequested = m_state->lifecycleSnapshot.dirtyFlags != 0U;
    if (!m_state->lifecycleSnapshot.persistRequested)
    {
        return clearDirty(DirtyFlag::Lifecycle);
    }

    return Result::Ok;
}

Result Context::registerDtc(const DtcId id, const DtcSeverity severity) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (m_state->config.dtcRecords == nullptr || m_state->config.dtcCapacity == 0U)
    {
        return Result::InvalidArgument;
    }

    if (findDtcRecord(m_state->config.dtcRecords, m_state->dtcCount, id) != nullptr)
    {
        return Result::AlreadyExists;
    }

    if (m_state->dtcCount >= m_state->config.dtcCapacity)
    {
        return Result::Capacity;
    }

    DtcRecord &record = m_state->config.dtcRecords[m_state->dtcCount];
    record = DtcRecord{};
    record.id = id;
    record.severity = severity;
    ++m_state->dtcCount;

    return markDirty(DirtyFlag::Dtc);
}

Result Context::listDtcs(DtcRecord *records, const std::size_t capacity,
                         std::size_t &count) const noexcept
{
    count = isInitialized() ? m_state->dtcCount : 0U;
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (capacity < m_state->dtcCount)
    {
        return Result::Capacity;
    }

    if (m_state->dtcCount > 0U && records == nullptr)
    {
        return Result::InvalidArgument;
    }

    for (std::size_t index = 0U; index < m_state->dtcCount; ++index)
    {
        records[index] = m_state->config.dtcRecords[index];
    }

    return Result::Ok;
}

Result Context::setDtcActive(const DtcId id, const bool active) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    DtcRecord *const record = findDtcRecord(m_state->config.dtcRecords, m_state->dtcCount, id);
    if (record == nullptr)
    {
        return Result::NotFound;
    }

    const bool wasActive = hasStatus(*record, DtcStatus::TestFailed);
    if (active)
    {
        record->status |= static_cast<DtcStatusFlags>(DtcStatus::TestFailed);
        record->status |= static_cast<DtcStatusFlags>(DtcStatus::Pending);
        record->status |= static_cast<DtcStatusFlags>(DtcStatus::Confirmed);
        record->status |= static_cast<DtcStatusFlags>(DtcStatus::TestFailedThisCycle);
        if (!wasActive && record->occurrenceCount < std::numeric_limits<std::uint32_t>::max())
        {
            ++record->occurrenceCount;
        }
    }
    else
    {
        record->status &= ~static_cast<DtcStatusFlags>(DtcStatus::TestFailed);
        record->status &= ~static_cast<DtcStatusFlags>(DtcStatus::TestFailedThisCycle);
    }

    if (active != wasActive)
    {
        return markDirty(DirtyFlag::Dtc);
    }

    return Result::Ok;
}

Result Context::clearDtc(const DtcId id) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    DtcRecord *const record = findDtcRecord(m_state->config.dtcRecords, m_state->dtcCount, id);
    if (record == nullptr)
    {
        return Result::NotFound;
    }

    if (record->status != 0U || record->occurrenceCount != 0U)
    {
        record->status = 0U;
        record->occurrenceCount = 0U;
        if (record->clearCount < std::numeric_limits<std::uint32_t>::max())
        {
            ++record->clearCount;
        }

        return markDirty(DirtyFlag::Dtc);
    }

    return Result::Ok;
}

} // namespace diag
