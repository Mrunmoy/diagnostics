#include "diag/context.hpp"

#include "byte_order.hpp"
#include "diag/capsule.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>

namespace diag
{

namespace
{

template <bool Enabled> struct DtcState : internal::EmptyFeature
{
};

template <> struct DtcState<true>
{
    std::size_t dtcCount{0U};
};

template <bool Enabled> struct IdentityState : internal::EmptyFeature
{
};

template <> struct IdentityState<true>
{
    Identity identity{};
    bool     identityAttached{false};
};

template <bool Enabled> struct LifecycleState : internal::EmptyFeature
{
};

template <> struct LifecycleState<true>
{
    LifecycleConfig   lifecycleConfig{};
    LifecycleSnapshot lifecycleSnapshot{};
    bool              lifecycleAttached{false};
};

template <bool Enabled> struct PersistenceState : internal::EmptyFeature
{
};

template <> struct PersistenceState<true>
{
    bool    storageAttached{false};
    bool    persistentLoadAttempted{false};
    Storage storage{};
};

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
constexpr std::size_t    kDtcPayloadHeaderSize = 4U;
constexpr std::size_t    kDtcPayloadRecordSize = DtcRecord::kEncodedSize;
constexpr DtcStatusFlags kKnownDtcStatusMask =
    static_cast<DtcStatusFlags>(DtcStatus::TestFailed) |
    static_cast<DtcStatusFlags>(DtcStatus::Pending) |
    static_cast<DtcStatusFlags>(DtcStatus::Confirmed) |
    static_cast<DtcStatusFlags>(DtcStatus::TestFailedThisCycle);
constexpr std::uint16_t kLifecycleCapsuleSectionVersion = 1U;
constexpr std::size_t   kLifecyclePayloadSize = 16U;

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

template <bool Enabled> struct DtcOps
{
    template <typename State> [[nodiscard]] static std::size_t count(const State &) noexcept
    {
        return 0U;
    }

    template <typename State>
    [[nodiscard]] static ResultValue<DtcRecord> get(const State &, DtcId) noexcept
    {
        return ResultValue<DtcRecord>{Result::NotSupported};
    }

    template <typename State>
    [[nodiscard]] static Result registerRecord(State &, DtcId, DtcSeverity) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static Result list(const State &, DtcRecord *, std::size_t,
                                     std::size_t &count) noexcept
    {
        count = 0U;
        return Result::NotSupported;
    }

    template <typename State> [[nodiscard]] static Result setActive(State &, DtcId, bool) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State> [[nodiscard]] static Result clear(State &, DtcId) noexcept
    {
        return Result::NotSupported;
    }
};

template <> struct DtcOps<true>
{
    template <typename State> [[nodiscard]] static std::size_t count(const State &state) noexcept
    {
        return state.dtcCount;
    }

    template <typename State>
    [[nodiscard]] static ResultValue<DtcRecord> get(const State &state, const DtcId id) noexcept
    {
        const DtcRecord *const record = findDtcRecord(state.config.dtcRecords, state.dtcCount, id);
        if (record == nullptr)
        {
            return ResultValue<DtcRecord>{Result::NotFound};
        }

        return ResultValue<DtcRecord>{*record};
    }

    template <typename State>
    [[nodiscard]] static Result registerRecord(State &state, const DtcId id,
                                               const DtcSeverity severity) noexcept
    {
        if (state.config.dtcRecords == nullptr || state.config.dtcCapacity == 0U)
        {
            return Result::InvalidArgument;
        }

        if (findDtcRecord(state.config.dtcRecords, state.dtcCount, id) != nullptr)
        {
            return Result::AlreadyExists;
        }

        if (state.dtcCount >= state.config.dtcCapacity)
        {
            return Result::Capacity;
        }

        DtcRecord &record = state.config.dtcRecords[state.dtcCount];
        record = DtcRecord{};
        record.id = id;
        record.severity = severity;
        ++state.dtcCount;
        state.dirtyFlags |= static_cast<DirtyFlags>(DirtyFlag::Dtc);

        return Result::Ok;
    }

    template <typename State>
    [[nodiscard]] static Result list(const State &state, DtcRecord *records,
                                     const std::size_t capacity, std::size_t &count) noexcept
    {
        count = state.dtcCount;
        if (capacity < state.dtcCount)
        {
            return Result::Capacity;
        }

        if (state.dtcCount > 0U && records == nullptr)
        {
            return Result::InvalidArgument;
        }

        for (std::size_t index = 0U; index < state.dtcCount; ++index)
        {
            records[index] = state.config.dtcRecords[index];
        }

        return Result::Ok;
    }

    template <typename State>
    [[nodiscard]] static Result setActive(State &state, const DtcId id, const bool active) noexcept
    {
        DtcRecord *const record = findDtcRecord(state.config.dtcRecords, state.dtcCount, id);
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
            state.dirtyFlags |= static_cast<DirtyFlags>(DirtyFlag::Dtc);
        }

        return Result::Ok;
    }

    template <typename State>
    [[nodiscard]] static Result clear(State &state, const DtcId id) noexcept
    {
        DtcRecord *const record = findDtcRecord(state.config.dtcRecords, state.dtcCount, id);
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

            state.dirtyFlags |= static_cast<DirtyFlags>(DirtyFlag::Dtc);
        }

        return Result::Ok;
    }
};

template <bool Enabled> struct IdentityOps
{
    template <typename State> [[nodiscard]] static ResultValue<Identity> get(const State &) noexcept
    {
        return ResultValue<Identity>{Result::NotSupported};
    }

    template <typename State> [[nodiscard]] static Result attach(State &, const Identity &) noexcept
    {
        return Result::NotSupported;
    }
};

template <> struct IdentityOps<true>
{
    template <typename State>
    [[nodiscard]] static ResultValue<Identity> get(const State &state) noexcept
    {
        if (!state.identityAttached)
        {
            return ResultValue<Identity>{Result::NotFound};
        }

        return ResultValue<Identity>{state.identity};
    }

    template <typename State>
    [[nodiscard]] static Result attach(State &state, const Identity &identity) noexcept
    {
        state.identity = identity;
        state.identityAttached = true;
        return Result::Ok;
    }
};

template <bool Enabled> struct LifecycleOps
{
    template <typename State>
    [[nodiscard]] static ResultValue<LifecycleSnapshot> get(const State &) noexcept
    {
        return ResultValue<LifecycleSnapshot>{Result::NotSupported};
    }

    template <typename State>
    [[nodiscard]] static Result attach(State &, const LifecycleConfig &) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static Result observeReset(State &, ResetReason) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static Result clearDirty(State &, LifecycleDirtyFlags) noexcept
    {
        return Result::NotSupported;
    }
};

template <> struct LifecycleOps<true>
{
    template <typename State>
    [[nodiscard]] static ResultValue<LifecycleSnapshot> get(const State &state) noexcept
    {
        if (!state.lifecycleAttached)
        {
            return ResultValue<LifecycleSnapshot>{Result::NotFound};
        }

        return ResultValue<LifecycleSnapshot>{state.lifecycleSnapshot};
    }

    template <typename State>
    [[nodiscard]] static Result attach(State &state, const LifecycleConfig &config) noexcept
    {
        state.lifecycleConfig = config;
        state.lifecycleSnapshot = LifecycleSnapshot{};
        state.lifecycleSnapshot.resetCounterPolicy = config.resetCounterPolicy;
        if (config.resetCounterPolicy == ResetCounterPolicy::Platform)
        {
            state.lifecycleSnapshot.resetCount = config.platformResetCount;
        }

        state.lifecycleAttached = true;
        state.dirtyFlags &= ~static_cast<DirtyFlags>(DirtyFlag::Lifecycle);

        if constexpr (features::kPersistence)
        {
            state.persistentLoadAttempted = false;
        }

        return Result::Ok;
    }

    template <typename State>
    [[nodiscard]] static Result observeReset(State &state, const ResetReason reason) noexcept
    {
        if (!state.lifecycleAttached)
        {
            return Result::NotFound;
        }

        LifecycleSnapshot       &snapshot = state.lifecycleSnapshot;
        const ResetCounterPolicy policy = state.lifecycleConfig.resetCounterPolicy;
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
                state.dirtyFlags |= static_cast<DirtyFlags>(DirtyFlag::Lifecycle);
            }
            break;

        case ResetCounterPolicy::EveryN:
            if (resetCountAdvanced && state.lifecycleConfig.resetCountInterval != 0U &&
                (snapshot.resetCount % state.lifecycleConfig.resetCountInterval) == 0U)
            {
                snapshot.dirtyFlags |=
                    static_cast<LifecycleDirtyFlags>(LifecycleDirtyFlag::ResetCounter);
                snapshot.persistRequested = true;
                state.dirtyFlags |= static_cast<DirtyFlags>(DirtyFlag::Lifecycle);
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

    template <typename State>
    [[nodiscard]] static Result clearDirty(State                    &state,
                                           const LifecycleDirtyFlags dirtyFlags) noexcept
    {
        if (!state.lifecycleAttached)
        {
            return Result::NotFound;
        }

        state.lifecycleSnapshot.dirtyFlags &= ~dirtyFlags;
        state.lifecycleSnapshot.persistRequested = state.lifecycleSnapshot.dirtyFlags != 0U;
        if (!state.lifecycleSnapshot.persistRequested)
        {
            state.dirtyFlags &= ~static_cast<DirtyFlags>(DirtyFlag::Lifecycle);
        }

        return Result::Ok;
    }
};

} // namespace

struct Context::State : DtcState<features::kDtc>,
                        IdentityState<features::kIdentity>,
                        LifecycleState<features::kLifecycle>,
                        PersistenceState<features::kPersistence>
{
    Config     config{};
    DirtyFlags dirtyFlags{0U};
    bool       initialized{false};
};

struct Context::StorageLayout
{
    bool engaged{false};
    alignas(State) std::uint8_t state[sizeof(State)];
};

namespace
{

template <bool Enabled> struct PersistenceOps
{
    template <typename State> [[nodiscard]] static Result attach(State &, const Storage &) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State> [[nodiscard]] static Result save(State &) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State> [[nodiscard]] static Result load(State &) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State> [[nodiscard]] static Result clear(State &) noexcept
    {
        return Result::NotSupported;
    }
};

template <> struct PersistenceOps<true>
{
    template <typename State>
    [[nodiscard]] static Result attach(State &state, const Storage &storage) noexcept
    {
        const Result validation = validateStorage(storage);
        if (validation != Result::Ok)
        {
            return validation;
        }

        if (storage.capsuleBuffer == nullptr || storage.capsuleBufferSize < kCapsuleHeaderSize)
        {
            return Result::InvalidArgument;
        }

        state.storage = storage;
        state.storageAttached = true;
        state.persistentLoadAttempted = false;
        return Result::Ok;
    }

    template <typename State>
    [[nodiscard]] static Result encodeDtcPayload(const State &state, std::uint8_t *payload,
                                                 const std::size_t capacity,
                                                 std::size_t      &usedLength) noexcept
    {
        usedLength = 0U;

        if (!features::kDtc)
        {
            return Result::NotSupported;
        }

        return encodeDtcPayloadImpl(state, payload, capacity, usedLength);
    }

    template <typename State>
    [[nodiscard]] static Result encodeLifecyclePayload(const State &state, std::uint8_t *payload,
                                                       const std::size_t capacity,
                                                       std::size_t      &usedLength) noexcept
    {
        usedLength = 0U;

        if (!features::kLifecycle)
        {
            return Result::NotSupported;
        }

        return encodeLifecyclePayloadImpl(state, payload, capacity, usedLength);
    }

    template <typename State> [[nodiscard]] static Result save(State &state) noexcept
    {
        if (!state.storageAttached)
        {
            return Result::NotFound;
        }

        if (state.dirtyFlags == 0U)
        {
            return Result::Ok;
        }

        Storage &storage = state.storage;

        const bool rawDtcDirty = (state.dirtyFlags & static_cast<DirtyFlags>(DirtyFlag::Dtc)) != 0U;
        const bool rawLifecycleDirty =
            (state.dirtyFlags & static_cast<DirtyFlags>(DirtyFlag::Lifecycle)) != 0U;

        if ((rawDtcDirty && !features::kDtc) || (rawLifecycleDirty && !features::kLifecycle))
        {
            return Result::NotSupported;
        }

        const bool dtcAttached = isDtcAttached(state);
        const bool lifecycleAttached = isLifecycleAttached(state);
        if ((rawDtcDirty && !dtcAttached) || (rawLifecycleDirty && !lifecycleAttached))
        {
            return Result::NotInitialized;
        }

        if (!rawDtcDirty && !rawLifecycleDirty)
        {
            return Result::NotSupported;
        }

        const bool dtcClean = dtcAttached && !rawDtcDirty;
        const bool lifecycleClean = lifecycleAttached && !rawLifecycleDirty;
        if (!state.persistentLoadAttempted && (dtcClean || lifecycleClean))
        {
            return Result::NotInitialized;
        }

        const bool    includeDtcSection = dtcAttached;
        const bool    includeLifecycleSection = lifecycleAttached;
        std::uint16_t sectionCount = 0U;
        if (includeDtcSection)
        {
            ++sectionCount;
        }

        if (includeLifecycleSection)
        {
            ++sectionCount;
        }

        if (sectionCount == 0U)
        {
            return Result::NotSupported;
        }

        const std::size_t payloadStart =
            kCapsuleHeaderSize +
            (static_cast<std::size_t>(sectionCount) * kCapsuleSectionEntrySize);
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
            const Result result =
                appendDtcSection(state, storage, descriptor, sectionIndex, payloadOffset);
            if (result != Result::Ok)
            {
                return result;
            }
        }

        if (includeLifecycleSection)
        {
            const Result result =
                appendLifecycleSection(state, storage, descriptor, sectionIndex, payloadOffset);
            if (result != Result::Ok)
            {
                return result;
            }
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

        const DirtyFlags savedFlags = state.dirtyFlags;
        const Result     saved = storageSave(storage, storage.capsuleBuffer, totalLength);
        if (saved == Result::Ok)
        {
            state.dirtyFlags &= ~savedFlags;
            clearSavedLifecycleState(state, savedFlags);
        }

        return saved;
    }

    template <typename State> [[nodiscard]] static Result load(State &state) noexcept
    {
        if (!state.storageAttached)
        {
            return Result::NotFound;
        }

        state.persistentLoadAttempted = true;

        Storage     &storage = state.storage;
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

        const ResultValue<CapsuleDescriptor> decoded =
            decodeCapsule(storage.capsuleBuffer, bytesRead);
        if (!decoded.hasValue())
        {
            return decoded.result();
        }

        const CapsuleDescriptor &descriptor = decoded.value();

        const Result dtcResult = loadDtcSection(state, storage, descriptor);
        if (dtcResult != Result::Ok)
        {
            return dtcResult;
        }

        return loadLifecycleSection(state, storage, descriptor);
    }

    template <typename State> [[nodiscard]] static Result clear(State &state) noexcept
    {
        if (!state.storageAttached)
        {
            return Result::NotFound;
        }

        const Result cleared = storageClear(state.storage);
        if (cleared == Result::Ok)
        {
            state.persistentLoadAttempted = true;
        }

        return cleared;
    }

  private:
    template <typename State> [[nodiscard]] static bool isDtcAttached(const State &state) noexcept
    {
        return isDtcAttachedImpl(state);
    }

    template <typename State>
    [[nodiscard]] static bool isLifecycleAttached(const State &state) noexcept
    {
        return isLifecycleAttachedImpl(state);
    }

    template <typename State>
    [[nodiscard]] static auto isDtcAttachedImpl(const State &state) noexcept
        -> decltype(state.config.dtcRecords, bool{})
    {
        return (state.config.dtcRecords != nullptr) && (state.config.dtcCapacity != 0U);
    }

    [[nodiscard]] static bool isDtcAttachedImpl(...) noexcept
    {
        return false;
    }

    template <typename State>
    [[nodiscard]] static auto isLifecycleAttachedImpl(const State &state) noexcept
        -> decltype(state.lifecycleAttached, bool{})
    {
        return state.lifecycleAttached;
    }

    [[nodiscard]] static bool isLifecycleAttachedImpl(...) noexcept
    {
        return false;
    }

    template <typename State>
    [[nodiscard]] static auto encodeDtcPayloadImpl(const State &state, std::uint8_t *payload,
                                                   const std::size_t capacity,
                                                   std::size_t      &usedLength) noexcept
        -> decltype(state.config.dtcRecords, Result{})
    {
        if (state.config.dtcRecords == nullptr || state.config.dtcCapacity == 0U)
        {
            return Result::NotInitialized;
        }

        if (state.dtcCount > (std::numeric_limits<std::uint16_t>::max)())
        {
            return Result::Capacity;
        }

        const std::size_t required =
            kDtcPayloadHeaderSize + (state.dtcCount * kDtcPayloadRecordSize);
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

    [[nodiscard]] static Result encodeDtcPayloadImpl(...) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static auto decodeDtcPayloadImpl(State &state, const std::uint8_t *payload,
                                                   const std::size_t length) noexcept
        -> decltype(state.config.dtcRecords, Result{})
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
                const std::size_t priorOffset =
                    kDtcPayloadHeaderSize + (prior * kDtcPayloadRecordSize);
                if (internal::readU32Le(&payload[priorOffset]) == idValue)
                {
                    return Result::CorruptData;
                }
            }
        }

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

    [[nodiscard]] static Result decodeDtcPayloadImpl(...) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static auto encodeLifecyclePayloadImpl(const State &state, std::uint8_t *payload,
                                                         const std::size_t capacity,
                                                         std::size_t      &usedLength) noexcept
        -> decltype(state.lifecycleAttached, Result{})
    {
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

    [[nodiscard]] static Result encodeLifecyclePayloadImpl(...) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static auto decodeLifecyclePayloadImpl(State &state, const std::uint8_t *payload,
                                                         const std::size_t length) noexcept
        -> decltype(state.lifecycleAttached, Result{})
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

    [[nodiscard]] static Result decodeLifecyclePayloadImpl(...) noexcept
    {
        return Result::NotSupported;
    }

    template <typename State>
    [[nodiscard]] static Result
    appendDtcSection(const State &state, const Storage &storage, CapsuleDescriptor &descriptor,
                     std::size_t &sectionIndex, std::size_t &payloadOffset) noexcept
    {
        std::size_t  usedLength = 0U;
        const Result result =
            encodeDtcPayload(state, &storage.capsuleBuffer[payloadOffset],
                             storage.capsuleBufferSize - payloadOffset, usedLength);
        if (result != Result::Ok)
        {
            return result;
        }

        return appendSection(storage, descriptor, sectionIndex, payloadOffset, usedLength,
                             CapsuleSectionType::ApplicationDtc, kDtcCapsuleSectionVersion);
    }

    template <typename State>
    [[nodiscard]] static Result appendLifecycleSection(const State &state, const Storage &storage,
                                                       CapsuleDescriptor &descriptor,
                                                       std::size_t       &sectionIndex,
                                                       std::size_t       &payloadOffset) noexcept
    {
        std::size_t  usedLength = 0U;
        const Result result =
            encodeLifecyclePayload(state, &storage.capsuleBuffer[payloadOffset],
                                   storage.capsuleBufferSize - payloadOffset, usedLength);
        if (result != Result::Ok)
        {
            return result;
        }

        return appendSection(storage, descriptor, sectionIndex, payloadOffset, usedLength,
                             CapsuleSectionType::Lifecycle, kLifecycleCapsuleSectionVersion);
    }

    [[nodiscard]] static Result appendSection(const Storage &storage, CapsuleDescriptor &descriptor,
                                              std::size_t &sectionIndex, std::size_t &payloadOffset,
                                              const std::size_t        usedLength,
                                              const CapsuleSectionType type,
                                              const std::uint16_t      version) noexcept
    {
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
        section.type = static_cast<std::uint16_t>(type);
        section.version = version;
        section.offset = static_cast<std::uint32_t>(payloadOffset);
        section.length = static_cast<std::uint32_t>(length);
        section.usedLength = static_cast<std::uint32_t>(usedLength);
        fillBytes(storage.capsuleBuffer, payloadOffset + usedLength, length - usedLength,
                  storage.capabilities.eraseValue);
        ++sectionIndex;
        payloadOffset += length;

        return Result::Ok;
    }

    template <typename State>
    [[nodiscard]] static Result loadDtcSection(State &state, const Storage &storage,
                                               const CapsuleDescriptor &descriptor) noexcept
    {
        const ResultValue<CapsuleSection> section = findCapsuleSectionByType(
            descriptor, static_cast<std::uint16_t>(CapsuleSectionType::ApplicationDtc));
        if (!section.hasValue())
        {
            return section.result() == Result::NotFound ? Result::Ok : section.result();
        }

        if (!features::kDtc)
        {
            return Result::Ok;
        }

        if (section.value().version != kDtcCapsuleSectionVersion)
        {
            return Result::CorruptData;
        }

        const Result result = decodeDtcPayloadImpl(
            state, &storage.capsuleBuffer[section.value().offset], section.value().usedLength);
        return result == Result::NotInitialized ? Result::Ok : result;
    }

    template <typename State>
    [[nodiscard]] static Result loadLifecycleSection(State &state, const Storage &storage,
                                                     const CapsuleDescriptor &descriptor) noexcept
    {
        const ResultValue<CapsuleSection> section = findCapsuleSectionByType(
            descriptor, static_cast<std::uint16_t>(CapsuleSectionType::Lifecycle));
        if (!section.hasValue())
        {
            return section.result() == Result::NotFound ? Result::Ok : section.result();
        }

        if (!features::kLifecycle)
        {
            return Result::Ok;
        }

        if (section.value().version != kLifecycleCapsuleSectionVersion)
        {
            return Result::CorruptData;
        }

        const Result result = decodeLifecyclePayloadImpl(
            state, &storage.capsuleBuffer[section.value().offset], section.value().usedLength);
        return result == Result::NotInitialized ? Result::Ok : result;
    }

    template <typename State>
    static auto clearSavedLifecycleStateImpl(State &state, DirtyFlags savedFlags) noexcept
        -> decltype(state.lifecycleSnapshot, void())
    {
        if ((savedFlags & static_cast<DirtyFlags>(DirtyFlag::Lifecycle)) != 0U)
        {
            state.lifecycleSnapshot.dirtyFlags = 0U;
            state.lifecycleSnapshot.persistRequested = false;
        }
    }

    static void clearSavedLifecycleStateImpl(...) noexcept {}

    template <typename State>
    static void clearSavedLifecycleState(State &state, const DirtyFlags savedFlags) noexcept
    {
        clearSavedLifecycleStateImpl(state, savedFlags);
    }
};

} // namespace

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

ResultValue<Identity> Context::identity() const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<Identity>{Result::NotInitialized};
    }

    return IdentityOps<features::kIdentity>::get(*m_state);
}

std::size_t Context::dtcCount() const noexcept
{
    return isInitialized() ? DtcOps<features::kDtc>::count(*m_state) : 0U;
}

ResultValue<DtcRecord> Context::dtc(const DtcId id) const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<DtcRecord>{Result::NotInitialized};
    }

    return DtcOps<features::kDtc>::get(*m_state, id);
}

ResultValue<LifecycleSnapshot> Context::lifecycle() const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<LifecycleSnapshot>{Result::NotInitialized};
    }

    return LifecycleOps<features::kLifecycle>::get(*m_state);
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

    return IdentityOps<features::kIdentity>::attach(*m_state, identity);
}

Result Context::attachLifecycle(const LifecycleConfig &config) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return LifecycleOps<features::kLifecycle>::attach(*m_state, config);
}

Result Context::observeReset(const ResetReason reason) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return LifecycleOps<features::kLifecycle>::observeReset(*m_state, reason);
}

Result Context::clearLifecycleDirty(const LifecycleDirtyFlags dirtyFlags) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return LifecycleOps<features::kLifecycle>::clearDirty(*m_state, dirtyFlags);
}

Result Context::registerDtc(const DtcId id, const DtcSeverity severity) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return DtcOps<features::kDtc>::registerRecord(*m_state, id, severity);
}

Result Context::listDtcs(DtcRecord *records, const std::size_t capacity,
                         std::size_t &count) const noexcept
{
    count = 0U;
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return DtcOps<features::kDtc>::list(*m_state, records, capacity, count);
}

Result Context::setDtcActive(const DtcId id, const bool active) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return DtcOps<features::kDtc>::setActive(*m_state, id, active);
}

Result Context::clearDtc(const DtcId id) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return DtcOps<features::kDtc>::clear(*m_state, id);
}

Result Context::attachStorage(const Storage &storage) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return PersistenceOps<features::kPersistence>::attach(*m_state, storage);
}

Result Context::savePersistent() noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return PersistenceOps<features::kPersistence>::save(*m_state);
}

Result Context::loadPersistent() noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return PersistenceOps<features::kPersistence>::load(*m_state);
}

Result Context::clearPersistent() noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    return PersistenceOps<features::kPersistence>::clear(*m_state);
}

} // namespace diag
