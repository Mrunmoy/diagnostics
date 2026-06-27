#pragma once

#include "diag/result.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace diag
{

constexpr std::uint16_t kCapsuleSchemaVersion = 1U;
constexpr std::size_t   kCapsuleMaxSections = 8U;
constexpr std::size_t   kCapsuleHeaderSize = 24U;
constexpr std::size_t   kCapsuleSectionEntrySize = 16U;
constexpr std::uint32_t kCapsuleMagic = 0x50434744U;

enum class CapsuleSectionType : std::uint16_t
{
    BootloaderDtc = 1U,
    ApplicationDtc = 2U,
    Lifecycle = 3U,
    Handoff = 4U,
    ResetCounters = 5U,
    Reserved = 0xFFFFU,
};

enum class CapsuleSectionOwner : std::uint8_t
{
    Unknown = 0U,
    Bootloader,
    Application,
    Shared,
    Reserved,
};

struct CapsuleSection
{
    std::uint16_t type{0U};
    std::uint16_t version{0U};
    std::uint32_t offset{0U};
    std::uint32_t length{0U};
    std::uint32_t usedLength{0U};
};

struct CapsuleDescriptor
{
    std::uint16_t                                   schemaVersion{kCapsuleSchemaVersion};
    std::uint16_t                                   sectionCount{0U};
    std::uint32_t                                   totalLength{0U};
    std::uint32_t                                   generation{0U};
    std::uint32_t                                   contentCrc32{0U};
    std::array<CapsuleSection, kCapsuleMaxSections> sections{};
};

struct CapsuleEncodeResult
{
    Result      result{Result::InvalidArgument};
    std::size_t encodedLength{0U};
};

struct CapsulePayloadCopyResult
{
    Result      result{Result::InvalidArgument};
    std::size_t copiedLength{0U};
};

[[nodiscard]] std::uint32_t capsuleCrc32(const std::uint8_t *data, std::size_t length) noexcept;

// Encodes only the fixed header and section table. Callers own payload bytes and
// must initialize every byte in [kCapsuleHeaderSize, descriptor.totalLength)
// deterministically before encoding because that whole range is covered by the
// capsule CRC, including unused section capacity and padding.
[[nodiscard]] CapsuleEncodeResult encodeCapsuleV1(std::uint8_t *buffer, std::size_t capacity,
                                                  const CapsuleDescriptor &descriptor) noexcept;
[[nodiscard]] ResultValue<CapsuleDescriptor> decodeCapsule(const std::uint8_t *buffer,
                                                           std::size_t         length) noexcept;
[[nodiscard]] CapsuleSectionOwner capsuleSectionOwnerFromType(std::uint16_t type) noexcept;
[[nodiscard]] ResultValue<CapsuleSection>
findCapsuleSectionByType(const CapsuleDescriptor &descriptor, std::uint16_t type) noexcept;
[[nodiscard]] ResultValue<CapsuleSection>
findCapsuleSectionByOwner(const CapsuleDescriptor &descriptor, CapsuleSectionOwner owner) noexcept;
[[nodiscard]] Result validateCapsuleSectionBounds(const CapsuleDescriptor &descriptor,
                                                  const CapsuleSection    &section) noexcept;
[[nodiscard]] CapsulePayloadCopyResult
copyCapsuleSectionPayload(const std::uint8_t *capsule, std::size_t capsuleLength,
                          const CapsuleDescriptor &descriptor, const CapsuleSection &section,
                          std::uint8_t *outPayload, std::size_t outCapacity) noexcept;
[[nodiscard]] CapsulePayloadCopyResult
copyCapsuleSectionPayloadByType(const std::uint8_t *capsule, std::size_t capsuleLength,
                                const CapsuleDescriptor &descriptor, std::uint16_t type,
                                std::uint8_t *outPayload, std::size_t outCapacity) noexcept;

} // namespace diag
