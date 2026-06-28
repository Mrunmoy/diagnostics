#include "diag/capsule.hpp"

#include <cstring>

namespace diag
{
namespace
{

void writeU16Le(std::uint8_t *const buffer, const std::uint16_t value) noexcept
{
    buffer[0] = static_cast<std::uint8_t>(value & 0xFFU);
    buffer[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void writeU32Le(std::uint8_t *const buffer, const std::uint32_t value) noexcept
{
    buffer[0] = static_cast<std::uint8_t>(value & 0xFFU);
    buffer[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buffer[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    buffer[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

[[nodiscard]] std::uint16_t readU16Le(const std::uint8_t *const buffer) noexcept
{
    return static_cast<std::uint16_t>(static_cast<std::uint16_t>(buffer[0]) |
                                      (static_cast<std::uint16_t>(buffer[1]) << 8U));
}

[[nodiscard]] std::uint32_t readU32Le(const std::uint8_t *const buffer) noexcept
{
    return static_cast<std::uint32_t>(static_cast<std::uint32_t>(buffer[0]) |
                                      (static_cast<std::uint32_t>(buffer[1]) << 8U) |
                                      (static_cast<std::uint32_t>(buffer[2]) << 16U) |
                                      (static_cast<std::uint32_t>(buffer[3]) << 24U));
}

[[nodiscard]] std::size_t sectionTableEnd(const std::uint16_t sectionCount) noexcept
{
    return kCapsuleHeaderSize + (static_cast<std::size_t>(sectionCount) * kCapsuleSectionEntrySize);
}

[[nodiscard]] bool rangePastEnd(const std::uint32_t offset, const std::uint32_t length,
                                const std::uint32_t totalLength) noexcept
{
    return offset > totalLength || length > (totalLength - offset);
}

[[nodiscard]] bool sectionsOverlap(const CapsuleSection &left, const CapsuleSection &right) noexcept
{
    const std::uint32_t leftEnd = left.offset + left.length;
    const std::uint32_t rightEnd = right.offset + right.length;

    return left.offset < rightEnd && right.offset < leftEnd;
}

[[nodiscard]] Result
validateSections(const std::array<CapsuleSection, kCapsuleMaxSections> &sections,
                 const std::uint16_t sectionCount, const std::uint32_t totalLength,
                 const std::size_t payloadStart) noexcept
{
    for (std::uint16_t index = 0U; index < sectionCount; ++index)
    {
        const CapsuleSection &section = sections[index];

        if (section.offset < payloadStart)
        {
            return Result::CorruptData;
        }

        if (section.usedLength > section.length)
        {
            return Result::CorruptData;
        }

        if (rangePastEnd(section.offset, section.length, totalLength))
        {
            return Result::CorruptData;
        }
    }

    for (std::uint16_t left = 0U; left < sectionCount; ++left)
    {
        for (std::uint16_t right = static_cast<std::uint16_t>(left + 1U); right < sectionCount;
             ++right)
        {
            if (sectionsOverlap(sections[left], sections[right]))
            {
                return Result::CorruptData;
            }
        }
    }

    return Result::Ok;
}

} // namespace

ResultValue<std::uint32_t> capsuleCrc32(const std::uint8_t *const data,
                                        const std::size_t         length) noexcept
{
    std::uint32_t crc = 0xFFFFFFFFU;

    if (data == nullptr && length != 0U)
    {
        return ResultValue<std::uint32_t>{Result::InvalidArgument};
    }

    for (std::size_t index = 0U; index < length; ++index)
    {
        crc ^= static_cast<std::uint32_t>(data[index]);
        for (std::uint8_t bit = 0U; bit < 8U; ++bit)
        {
            const std::uint32_t mask = static_cast<std::uint32_t>(0U - (crc & 1U));
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }

    return ResultValue<std::uint32_t>{~crc};
}

CapsuleEncodeResult encodeCapsuleV1(std::uint8_t *const buffer, const std::size_t capacity,
                                    const CapsuleDescriptor &descriptor) noexcept
{
    if (buffer == nullptr)
    {
        return CapsuleEncodeResult{Result::InvalidArgument, 0U};
    }

    if (descriptor.schemaVersion != kCapsuleSchemaVersion)
    {
        return CapsuleEncodeResult{Result::InvalidArgument, 0U};
    }

    if (descriptor.sectionCount > kCapsuleMaxSections)
    {
        return CapsuleEncodeResult{Result::Capacity, 0U};
    }

    const std::size_t payloadStart = sectionTableEnd(descriptor.sectionCount);
    if (descriptor.totalLength < payloadStart || capacity < descriptor.totalLength)
    {
        return CapsuleEncodeResult{Result::Capacity, 0U};
    }

    if (validateSections(descriptor.sections, descriptor.sectionCount, descriptor.totalLength,
                         payloadStart) != Result::Ok)
    {
        return CapsuleEncodeResult{Result::InvalidArgument, 0U};
    }

    writeU32Le(&buffer[0], kCapsuleMagic);
    writeU16Le(&buffer[4], kCapsuleSchemaVersion);
    writeU16Le(&buffer[6], static_cast<std::uint16_t>(kCapsuleHeaderSize));
    writeU32Le(&buffer[8], descriptor.totalLength);
    writeU32Le(&buffer[12], descriptor.generation);
    writeU16Le(&buffer[16], descriptor.sectionCount);
    writeU16Le(&buffer[18], 0U);
    writeU32Le(&buffer[20], 0U);

    for (std::uint16_t index = 0U; index < descriptor.sectionCount; ++index)
    {
        const CapsuleSection &section = descriptor.sections[index];
        const std::size_t     entryOffset =
            kCapsuleHeaderSize + (static_cast<std::size_t>(index) * kCapsuleSectionEntrySize);

        writeU16Le(&buffer[entryOffset], section.type);
        writeU16Le(&buffer[entryOffset + 2U], section.version);
        writeU32Le(&buffer[entryOffset + 4U], section.offset);
        writeU32Le(&buffer[entryOffset + 8U], section.length);
        writeU32Le(&buffer[entryOffset + 12U], section.usedLength);
    }

    const ResultValue<std::uint32_t> contentCrc32 =
        capsuleCrc32(&buffer[kCapsuleHeaderSize], descriptor.totalLength - kCapsuleHeaderSize);
    if (!contentCrc32.hasValue())
    {
        return CapsuleEncodeResult{contentCrc32.result(), 0U};
    }

    writeU32Le(&buffer[20], contentCrc32.value());

    return CapsuleEncodeResult{Result::Ok, descriptor.totalLength};
}

ResultValue<CapsuleDescriptor> decodeCapsule(const std::uint8_t *const buffer,
                                             const std::size_t         length) noexcept
{
    if (buffer == nullptr)
    {
        return ResultValue<CapsuleDescriptor>{Result::InvalidArgument};
    }

    if (length < kCapsuleHeaderSize)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    if (readU32Le(&buffer[0]) != kCapsuleMagic)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    if (readU16Le(&buffer[4]) != kCapsuleSchemaVersion)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    if (readU16Le(&buffer[6]) != kCapsuleHeaderSize)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    const std::uint32_t totalLength = readU32Le(&buffer[8]);
    if (totalLength > length)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    const std::uint16_t sectionCount = readU16Le(&buffer[16]);
    if (sectionCount > kCapsuleMaxSections)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    const std::size_t payloadStart = sectionTableEnd(sectionCount);
    if (totalLength < payloadStart)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    CapsuleDescriptor descriptor{};
    descriptor.schemaVersion = kCapsuleSchemaVersion;
    descriptor.sectionCount = sectionCount;
    descriptor.totalLength = totalLength;
    descriptor.generation = readU32Le(&buffer[12]);
    descriptor.contentCrc32 = readU32Le(&buffer[20]);

    for (std::uint16_t index = 0U; index < sectionCount; ++index)
    {
        CapsuleSection   &section = descriptor.sections[index];
        const std::size_t entryOffset =
            kCapsuleHeaderSize + (static_cast<std::size_t>(index) * kCapsuleSectionEntrySize);

        section.type = readU16Le(&buffer[entryOffset]);
        section.version = readU16Le(&buffer[entryOffset + 2U]);
        section.offset = readU32Le(&buffer[entryOffset + 4U]);
        section.length = readU32Le(&buffer[entryOffset + 8U]);
        section.usedLength = readU32Le(&buffer[entryOffset + 12U]);
    }

    if (validateSections(descriptor.sections, sectionCount, totalLength, payloadStart) !=
        Result::Ok)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    const ResultValue<std::uint32_t> actualContentCrc32 =
        capsuleCrc32(&buffer[kCapsuleHeaderSize], totalLength - kCapsuleHeaderSize);
    if (!actualContentCrc32.hasValue())
    {
        return ResultValue<CapsuleDescriptor>{actualContentCrc32.result()};
    }

    if (actualContentCrc32.value() != descriptor.contentCrc32)
    {
        return ResultValue<CapsuleDescriptor>{Result::CorruptData};
    }

    return ResultValue<CapsuleDescriptor>{descriptor};
}

CapsuleSectionOwner capsuleSectionOwnerFromType(const std::uint16_t type) noexcept
{
    switch (type)
    {
    case static_cast<std::uint16_t>(CapsuleSectionType::BootloaderDtc):
        return CapsuleSectionOwner::Bootloader;

    case static_cast<std::uint16_t>(CapsuleSectionType::ApplicationDtc):
        return CapsuleSectionOwner::Application;

    case static_cast<std::uint16_t>(CapsuleSectionType::Lifecycle):
    case static_cast<std::uint16_t>(CapsuleSectionType::Handoff):
    case static_cast<std::uint16_t>(CapsuleSectionType::ResetCounters):
        return CapsuleSectionOwner::Shared;

    case static_cast<std::uint16_t>(CapsuleSectionType::Reserved):
        return CapsuleSectionOwner::Reserved;

    default:
        return CapsuleSectionOwner::Unknown;
    }
}

ResultValue<CapsuleSection> findCapsuleSectionByType(const CapsuleDescriptor &descriptor,
                                                     const std::uint16_t      type) noexcept
{
    if (descriptor.sectionCount > kCapsuleMaxSections)
    {
        return ResultValue<CapsuleSection>{Result::CorruptData};
    }

    for (std::uint16_t index = 0U; index < descriptor.sectionCount; ++index)
    {
        const CapsuleSection &section = descriptor.sections[index];
        if (section.type == type)
        {
            return ResultValue<CapsuleSection>{section};
        }
    }

    return ResultValue<CapsuleSection>{Result::NotFound};
}

ResultValue<CapsuleSection> findCapsuleSectionByOwner(const CapsuleDescriptor  &descriptor,
                                                      const CapsuleSectionOwner owner) noexcept
{
    if (descriptor.sectionCount > kCapsuleMaxSections)
    {
        return ResultValue<CapsuleSection>{Result::CorruptData};
    }

    for (std::uint16_t index = 0U; index < descriptor.sectionCount; ++index)
    {
        const CapsuleSection &section = descriptor.sections[index];
        if (capsuleSectionOwnerFromType(section.type) == owner)
        {
            return ResultValue<CapsuleSection>{section};
        }
    }

    return ResultValue<CapsuleSection>{Result::NotFound};
}

Result validateCapsuleSectionBounds(const CapsuleDescriptor &descriptor,
                                    const CapsuleSection    &section) noexcept
{
    if (descriptor.sectionCount > kCapsuleMaxSections)
    {
        return Result::CorruptData;
    }

    const std::size_t payloadStart = sectionTableEnd(descriptor.sectionCount);
    if (descriptor.totalLength < payloadStart)
    {
        return Result::CorruptData;
    }

    if (section.offset < payloadStart)
    {
        return Result::CorruptData;
    }

    if (section.usedLength > section.length)
    {
        return Result::CorruptData;
    }

    if (rangePastEnd(section.offset, section.length, descriptor.totalLength))
    {
        return Result::CorruptData;
    }

    return Result::Ok;
}

CapsulePayloadCopyResult
copyCapsuleSectionPayload(const std::uint8_t *const capsule, const std::size_t capsuleLength,
                          const CapsuleDescriptor &descriptor, const CapsuleSection &section,
                          std::uint8_t *const outPayload, const std::size_t outCapacity) noexcept
{
    if (capsule == nullptr || outPayload == nullptr)
    {
        return CapsulePayloadCopyResult{Result::InvalidArgument, 0U};
    }

    const Result boundsResult = validateCapsuleSectionBounds(descriptor, section);
    if (boundsResult != Result::Ok)
    {
        return CapsulePayloadCopyResult{boundsResult, 0U};
    }

    if (descriptor.totalLength > capsuleLength)
    {
        return CapsulePayloadCopyResult{Result::CorruptData, 0U};
    }

    if (section.usedLength > outCapacity)
    {
        return CapsulePayloadCopyResult{Result::Capacity, 0U};
    }

    if (section.usedLength != 0U)
    {
        std::memcpy(outPayload, &capsule[section.offset], section.usedLength);
    }

    return CapsulePayloadCopyResult{Result::Ok, section.usedLength};
}

CapsulePayloadCopyResult copyCapsuleSectionPayloadByType(const std::uint8_t *const capsule,
                                                         const std::size_t         capsuleLength,
                                                         const CapsuleDescriptor  &descriptor,
                                                         const std::uint16_t       type,
                                                         std::uint8_t *const       outPayload,
                                                         const std::size_t outCapacity) noexcept
{
    const ResultValue<CapsuleSection> section = findCapsuleSectionByType(descriptor, type);
    if (!section.hasValue())
    {
        return CapsulePayloadCopyResult{section.result(), 0U};
    }

    return copyCapsuleSectionPayload(capsule, capsuleLength, descriptor, section.value(),
                                     outPayload, outCapacity);
}

} // namespace diag
