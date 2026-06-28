#include "diag/context.hpp"

#include "byte_order.hpp"
#include "diag/capsule.hpp"

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
    bool              storageAttached{false};
    bool              persistentLoadAttempted{false};
    Storage           storage{};
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

constexpr std::uint16_t  kDtcCapsuleSectionVersion = 1U;
constexpr std::uint16_t  kLifecycleCapsuleSectionVersion = 1U;
constexpr std::size_t    kDtcPayloadHeaderSize = 4U;
constexpr std::size_t    kDtcPayloadRecordSize = DtcRecord::kEncodedSize;
constexpr std::size_t    kLifecyclePayloadSize = 16U;
constexpr DtcStatusFlags kKnownDtcStatusMask =
    static_cast<DtcStatusFlags>(DtcStatus::TestFailed) |
    static_cast<DtcStatusFlags>(DtcStatus::Pending) |
    static_cast<DtcStatusFlags>(DtcStatus::Confirmed) |
    static_cast<DtcStatusFlags>(DtcStatus::TestFailedThisCycle);

[[nodiscard]] ResultValue<std::size_t> alignUp(const std::size_t value,
                                               const std::size_t alignment) noexcept
{
    const std::size_t remainder = value % alignment;
    if (remainder == 0U)
    {
        return ResultValue<std::size_t>{value};
    }

    const std::size_t padding = alignment - remainder;
    if (value > ((std::numeric_limits<std::size_t>::max)() - padding))
    {
        return ResultValue<std::size_t>{Result::Capacity};
    }

    return ResultValue<std::size_t>{value + padding};
}

void fillBytes(std::uint8_t *const buffer, const std::size_t offset, const std::size_t length,
               const std::uint8_t value) noexcept
{
    for (std::size_t index = 0U; index < length; ++index)
    {
        buffer[offset + index] = value;
    }
}

} // namespace

Result Context::encodeDtcPayload(const State &state, std::uint8_t *const payload,
                                 const std::size_t capacity, std::size_t &usedLength) noexcept
{
    usedLength = 0U;

    if (state.config.dtcRecords == nullptr)
    {
        return Result::NotInitialized;
    }

    if (state.dtcCount > (std::numeric_limits<std::uint16_t>::max)())
    {
        return Result::Capacity;
    }

    const std::size_t required = kDtcPayloadHeaderSize + (state.dtcCount * kDtcPayloadRecordSize);
    if (payload == nullptr || capacity < required)
    {
        return Result::Capacity;
    }

    internal::writeU16Le(&payload[0], static_cast<std::uint16_t>(state.dtcCount));
    internal::writeU16Le(&payload[2], static_cast<std::uint16_t>(kDtcPayloadRecordSize));

    for (std::size_t index = 0U; index < state.dtcCount; ++index)
    {
        const DtcRecord  &record = state.config.dtcRecords[index];
        const std::size_t offset = kDtcPayloadHeaderSize + (index * kDtcPayloadRecordSize);

        internal::writeU32Le(&payload[offset], record.id.value);
        internal::writeU32Le(&payload[offset + 4U], record.occurrenceCount);
        internal::writeU32Le(&payload[offset + 8U], record.clearCount);
        payload[offset + 12U] = record.status;
        payload[offset + 13U] = static_cast<std::uint8_t>(record.severity);
        payload[offset + 14U] = 0U;
        payload[offset + 15U] = 0U;
    }

    usedLength = required;
    return Result::Ok;
}

Result Context::decodeDtcPayload(State &state, const std::uint8_t *const payload,
                                 const std::size_t length) noexcept
{
    if (state.config.dtcRecords == nullptr || state.config.dtcCapacity == 0U)
    {
        return Result::NotInitialized;
    }

    if (payload == nullptr || length < kDtcPayloadHeaderSize)
    {
        return Result::CorruptData;
    }

    const std::uint16_t count = internal::readU16Le(&payload[0]);
    const std::uint16_t recordSize = internal::readU16Le(&payload[2]);
    const std::size_t   expected =
        kDtcPayloadHeaderSize + (static_cast<std::size_t>(count) * kDtcPayloadRecordSize);

    if (recordSize != kDtcPayloadRecordSize || length != expected)
    {
        return Result::CorruptData;
    }

    if (count > state.config.dtcCapacity)
    {
        return Result::Capacity;
    }

    // First pass: validate the payload without mutating state.
    for (std::size_t index = 0U; index < count; ++index)
    {
        const std::size_t offset = kDtcPayloadHeaderSize + (index * kDtcPayloadRecordSize);
        if ((payload[offset + 12U] & ~kKnownDtcStatusMask) != 0U ||
            payload[offset + 13U] > static_cast<std::uint8_t>(DtcSeverity::Critical) ||
            payload[offset + 14U] != 0U || payload[offset + 15U] != 0U)
        {
            return Result::CorruptData;
        }

        const std::uint32_t idValue = internal::readU32Le(&payload[offset]);
        for (std::size_t prior = 0U; prior < index; ++prior)
        {
            const std::size_t priorOffset = kDtcPayloadHeaderSize + (prior * kDtcPayloadRecordSize);
            if (internal::readU32Le(&payload[priorOffset]) == idValue)
            {
                return Result::CorruptData;
            }
        }
    }

    // Second pass: apply decoded records.
    for (std::size_t index = 0U; index < count; ++index)
    {
        const std::size_t offset = kDtcPayloadHeaderSize + (index * kDtcPayloadRecordSize);
        DtcRecord        &record = state.config.dtcRecords[index];
        record.id = DtcId{internal::readU32Le(&payload[offset])};
        record.occurrenceCount = internal::readU32Le(&payload[offset + 4U]);
        record.clearCount = internal::readU32Le(&payload[offset + 8U]);
        record.status = payload[offset + 12U];
        record.severity = static_cast<DtcSeverity>(payload[offset + 13U]);
        record.reserved0 = 0U;
        record.reserved1 = 0U;
    }

    state.dtcCount = count;
    state.dirtyFlags &= ~static_cast<DirtyFlags>(DirtyFlag::Dtc);
    return Result::Ok;
}

Result Context::encodeLifecyclePayload(const State &state, std::uint8_t *const payload,
                                       const std::size_t capacity, std::size_t &usedLength) noexcept
{
    usedLength = 0U;

    if (!state.lifecycleAttached)
    {
        return Result::NotInitialized;
    }

    if (payload == nullptr || capacity < kLifecyclePayloadSize)
    {
        return Result::Capacity;
    }

    if (static_cast<std::uint8_t>(state.lifecycleSnapshot.lastResetReason) >
            static_cast<std::uint8_t>(ResetReason::Fault) ||
        static_cast<std::uint8_t>(state.lifecycleSnapshot.resetCounterPolicy) >
            static_cast<std::uint8_t>(ResetCounterPolicy::Platform))
    {
        return Result::InvalidArgument;
    }

    internal::writeU16Le(&payload[0], static_cast<std::uint16_t>(kLifecyclePayloadSize));
    payload[2] = static_cast<std::uint8_t>(state.lifecycleSnapshot.lastResetReason);
    payload[3] = static_cast<std::uint8_t>(state.lifecycleSnapshot.resetCounterPolicy);
    internal::writeU32Le(&payload[4], state.lifecycleSnapshot.resetCount);
    internal::writeU32Le(&payload[8], state.lifecycleSnapshot.abnormalResetCount);
    internal::writeU32Le(&payload[12], 0U);

    usedLength = kLifecyclePayloadSize;
    return Result::Ok;
}

Result Context::decodeLifecyclePayload(State &state, const std::uint8_t *const payload,
                                       const std::size_t length) noexcept
{
    if (!state.lifecycleAttached)
    {
        return Result::NotInitialized;
    }

    if (payload == nullptr || length != kLifecyclePayloadSize)
    {
        return Result::CorruptData;
    }

    if (internal::readU16Le(&payload[0]) != kLifecyclePayloadSize ||
        payload[2] > static_cast<std::uint8_t>(ResetReason::Fault) ||
        payload[3] > static_cast<std::uint8_t>(ResetCounterPolicy::Platform) ||
        internal::readU32Le(&payload[12]) != 0U)
    {
        return Result::CorruptData;
    }

    const ResetCounterPolicy persistedPolicy = static_cast<ResetCounterPolicy>(payload[3]);
    if (persistedPolicy != state.lifecycleConfig.resetCounterPolicy)
    {
        return Result::CorruptData;
    }

    state.lifecycleSnapshot.lastResetReason = static_cast<ResetReason>(payload[2]);
    state.lifecycleSnapshot.resetCounterPolicy = state.lifecycleConfig.resetCounterPolicy;
    state.lifecycleSnapshot.resetCount = internal::readU32Le(&payload[4]);
    state.lifecycleSnapshot.abnormalResetCount = internal::readU32Le(&payload[8]);
    state.lifecycleSnapshot.dirtyFlags = 0U;
    state.lifecycleSnapshot.persistRequested = false;
    state.dirtyFlags &= ~static_cast<DirtyFlags>(DirtyFlag::Lifecycle);
    return Result::Ok;
}

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
        return ResultValue<LifecycleSnapshot>{Result::NotFound};
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
        return Result::NotFound;
    }

    LifecycleSnapshot       &snapshot = m_state->lifecycleSnapshot;
    const ResetCounterPolicy policy = m_state->lifecycleConfig.resetCounterPolicy;
    const bool               abnormal = isAbnormalReset(reason);
    bool                     resetCountAdvanced = false;

    snapshot.lastResetReason = reason;

    if (policy != ResetCounterPolicy::Disabled && policy != ResetCounterPolicy::Platform)
    {
        if (snapshot.resetCount < std::numeric_limits<std::uint32_t>::max())
        {
            ++snapshot.resetCount;
            resetCountAdvanced = true;
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
        if (resetCountAdvanced && m_state->lifecycleConfig.resetCountInterval != 0U &&
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
        return Result::NotFound;
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

Result Context::attachStorage(const Storage &storage) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    const Result validation = validateStorage(storage);
    if (validation != Result::Ok)
    {
        return validation;
    }

    if (storage.capsuleBuffer == nullptr || storage.capsuleBufferSize < kCapsuleHeaderSize)
    {
        return Result::InvalidArgument;
    }

    m_state->storage = storage;
    m_state->storageAttached = true;
    m_state->persistentLoadAttempted = false;
    return Result::Ok;
}

Result Context::savePersistent() noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (!m_state->storageAttached)
    {
        return Result::NotFound;
    }

    if (m_state->dirtyFlags == 0U)
    {
        return Result::Ok;
    }

    Storage &storage = m_state->storage;
    if (storage.capsuleBuffer == nullptr || storage.capsuleBufferSize < kCapsuleHeaderSize)
    {
        return Result::InvalidArgument;
    }

    const bool dtcDirty = (m_state->dirtyFlags & static_cast<DirtyFlags>(DirtyFlag::Dtc)) != 0U;
    const bool lifecycleDirty =
        (m_state->dirtyFlags & static_cast<DirtyFlags>(DirtyFlag::Lifecycle)) != 0U;
    if ((dtcDirty && m_state->config.dtcRecords == nullptr) ||
        (lifecycleDirty && !m_state->lifecycleAttached))
    {
        return Result::NotInitialized;
    }

    const bool dtcClean = (m_state->config.dtcRecords != nullptr) && !dtcDirty;
    const bool lifecycleClean = m_state->lifecycleAttached && !lifecycleDirty;
    // Guard against overwriting previously persisted data with default in-memory values.
    // Callers must invoke loadPersistent() after attaching storage before the first save.
    if (!m_state->persistentLoadAttempted && (dtcClean || lifecycleClean))
    {
        return Result::NotInitialized;
    }

    const bool    includeDtcSection = m_state->config.dtcRecords != nullptr;
    const bool    includeLifecycleSection = m_state->lifecycleAttached;
    std::uint16_t sectionCount = 0U;
    if (includeDtcSection)
    {
        ++sectionCount;
    }

    if (includeLifecycleSection)
    {
        ++sectionCount;
    }

    const std::size_t payloadStart =
        kCapsuleHeaderSize + (static_cast<std::size_t>(sectionCount) * kCapsuleSectionEntrySize);
    if (storage.capsuleBufferSize < payloadStart)
    {
        return Result::Capacity;
    }

    CapsuleDescriptor descriptor{};
    descriptor.schemaVersion = kCapsuleSchemaVersion;
    descriptor.sectionCount = sectionCount;
    descriptor.generation = 0U;

    std::size_t payloadOffset = payloadStart;
    std::size_t sectionIndex = 0U;

    if (includeDtcSection)
    {
        std::size_t  usedLength = 0U;
        const Result result =
            encodeDtcPayload(*m_state, &storage.capsuleBuffer[payloadOffset],
                             storage.capsuleBufferSize - payloadOffset, usedLength);
        if (result != Result::Ok)
        {
            return result;
        }

        const ResultValue<std::size_t> alignedLength =
            alignUp(usedLength, storage.capabilities.writeAlignment);
        if (!alignedLength.hasValue())
        {
            return alignedLength.result();
        }

        const std::size_t length = alignedLength.value();
        if (length > (storage.capsuleBufferSize - payloadOffset) ||
            payloadOffset > (std::numeric_limits<std::uint32_t>::max)() ||
            length > (std::numeric_limits<std::uint32_t>::max)() ||
            usedLength > (std::numeric_limits<std::uint32_t>::max)())
        {
            return Result::Capacity;
        }

        CapsuleSection &section = descriptor.sections[sectionIndex];
        section.type = static_cast<std::uint16_t>(CapsuleSectionType::ApplicationDtc);
        section.version = kDtcCapsuleSectionVersion;
        section.offset = static_cast<std::uint32_t>(payloadOffset);
        section.length = static_cast<std::uint32_t>(length);
        section.usedLength = static_cast<std::uint32_t>(usedLength);
        fillBytes(storage.capsuleBuffer, payloadOffset + usedLength, length - usedLength,
                  storage.capabilities.eraseValue);
        ++sectionIndex;
        payloadOffset += length;
    }

    if (includeLifecycleSection)
    {
        std::size_t  usedLength = 0U;
        const Result result =
            encodeLifecyclePayload(*m_state, &storage.capsuleBuffer[payloadOffset],
                                   storage.capsuleBufferSize - payloadOffset, usedLength);
        if (result != Result::Ok)
        {
            return result;
        }

        const ResultValue<std::size_t> alignedLength =
            alignUp(usedLength, storage.capabilities.writeAlignment);
        if (!alignedLength.hasValue())
        {
            return alignedLength.result();
        }

        const std::size_t length = alignedLength.value();
        if (length > (storage.capsuleBufferSize - payloadOffset) ||
            payloadOffset > (std::numeric_limits<std::uint32_t>::max)() ||
            length > (std::numeric_limits<std::uint32_t>::max)() ||
            usedLength > (std::numeric_limits<std::uint32_t>::max)())
        {
            return Result::Capacity;
        }

        CapsuleSection &section = descriptor.sections[sectionIndex];
        section.type = static_cast<std::uint16_t>(CapsuleSectionType::Lifecycle);
        section.version = kLifecycleCapsuleSectionVersion;
        section.offset = static_cast<std::uint32_t>(payloadOffset);
        section.length = static_cast<std::uint32_t>(length);
        section.usedLength = static_cast<std::uint32_t>(usedLength);
        fillBytes(storage.capsuleBuffer, payloadOffset + usedLength, length - usedLength,
                  storage.capabilities.eraseValue);
        ++sectionIndex;
        payloadOffset += length;
    }

    const ResultValue<std::size_t> alignedTotalLength =
        alignUp(payloadOffset, storage.capabilities.writeAlignment);
    if (!alignedTotalLength.hasValue())
    {
        return alignedTotalLength.result();
    }

    const std::size_t totalLength = alignedTotalLength.value();
    if (totalLength > storage.capsuleBufferSize ||
        totalLength > (std::numeric_limits<std::uint32_t>::max)())
    {
        return Result::Capacity;
    }

    descriptor.totalLength = static_cast<std::uint32_t>(totalLength);
    fillBytes(storage.capsuleBuffer, payloadOffset, totalLength - payloadOffset,
              storage.capabilities.eraseValue);

    const CapsuleEncodeResult encode =
        encodeCapsuleV1(storage.capsuleBuffer, storage.capsuleBufferSize, descriptor);
    if (encode.result != Result::Ok)
    {
        return encode.result;
    }

    const DirtyFlags savedFlags = m_state->dirtyFlags;
    const Result     saved = storageSave(storage, storage.capsuleBuffer, totalLength);
    if (saved == Result::Ok)
    {
        m_state->dirtyFlags &= ~savedFlags;
        if ((savedFlags & static_cast<DirtyFlags>(DirtyFlag::Lifecycle)) != 0U)
        {
            m_state->lifecycleSnapshot.dirtyFlags = 0U;
            m_state->lifecycleSnapshot.persistRequested = false;
        }
    }

    return saved;
}

Result Context::loadPersistent() noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (!m_state->storageAttached)
    {
        return Result::NotFound;
    }

    m_state->persistentLoadAttempted = true;

    Storage &storage = m_state->storage;
    if (storage.capsuleBuffer == nullptr || storage.capsuleBufferSize == 0U)
    {
        return Result::InvalidArgument;
    }

    std::size_t  bytesRead = 0U;
    const Result loaded =
        storageLoad(storage, storage.capsuleBuffer, storage.capsuleBufferSize, bytesRead);
    if (loaded != Result::Ok)
    {
        return loaded;
    }

    if (bytesRead == 0U)
    {
        return Result::Ok;
    }

    const ResultValue<CapsuleDescriptor> decoded = decodeCapsule(storage.capsuleBuffer, bytesRead);
    if (!decoded.hasValue())
    {
        return decoded.result();
    }

    const CapsuleDescriptor &descriptor = decoded.value();

    const ResultValue<CapsuleSection> dtcSection = findCapsuleSectionByType(
        descriptor, static_cast<std::uint16_t>(CapsuleSectionType::ApplicationDtc));
    if (dtcSection.hasValue())
    {
        if (dtcSection.value().version != kDtcCapsuleSectionVersion)
        {
            return Result::CorruptData;
        }

        const Result result =
            decodeDtcPayload(*m_state, &storage.capsuleBuffer[dtcSection.value().offset],
                             dtcSection.value().usedLength);
        if (result != Result::Ok && result != Result::NotInitialized)
        {
            return result;
        }
    }
    else if (dtcSection.result() != Result::NotFound)
    {
        return dtcSection.result();
    }

    const ResultValue<CapsuleSection> lifecycleSection = findCapsuleSectionByType(
        descriptor, static_cast<std::uint16_t>(CapsuleSectionType::Lifecycle));
    if (lifecycleSection.hasValue())
    {
        if (lifecycleSection.value().version != kLifecycleCapsuleSectionVersion)
        {
            return Result::CorruptData;
        }

        const Result result = decodeLifecyclePayload(
            *m_state, &storage.capsuleBuffer[lifecycleSection.value().offset],
            lifecycleSection.value().usedLength);
        if (result != Result::Ok && result != Result::NotInitialized)
        {
            return result;
        }
    }
    else if (lifecycleSection.result() != Result::NotFound)
    {
        return lifecycleSection.result();
    }

    return Result::Ok;
}

Result Context::clearPersistent() noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    if (!m_state->storageAttached)
    {
        return Result::NotFound;
    }

    const Result cleared = storageClear(m_state->storage);
    if (cleared == Result::Ok)
    {
        m_state->persistentLoadAttempted = true;
    }

    return cleared;
}

} // namespace diag
