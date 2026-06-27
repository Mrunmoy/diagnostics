#include "diag/diag.hpp"

#include <array>
#include <cstdint>

#include <gtest/gtest.h>

namespace
{

constexpr std::size_t kCapsuleBytes = 128U;

diag::CapsuleDescriptor makeOneSectionDescriptor()
{
    diag::CapsuleDescriptor descriptor{};
    descriptor.schemaVersion = diag::kCapsuleSchemaVersion;
    descriptor.sectionCount = 1U;
    descriptor.totalLength = kCapsuleBytes;
    descriptor.generation = 7U;
    descriptor.sections[0].type =
        static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc);
    descriptor.sections[0].version = 1U;
    descriptor.sections[0].offset = diag::kCapsuleHeaderSize + diag::kCapsuleSectionEntrySize;
    descriptor.sections[0].length = 16U;
    descriptor.sections[0].usedLength = 4U;
    return descriptor;
}

} // namespace

TEST(DiagCapsule, MapsSectionTypesToOwners)
{
    EXPECT_EQ(diag::capsuleSectionOwnerFromType(
                  static_cast<std::uint16_t>(diag::CapsuleSectionType::BootloaderDtc)),
              diag::CapsuleSectionOwner::Bootloader);
    EXPECT_EQ(diag::capsuleSectionOwnerFromType(
                  static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc)),
              diag::CapsuleSectionOwner::Application);
    EXPECT_EQ(diag::capsuleSectionOwnerFromType(
                  static_cast<std::uint16_t>(diag::CapsuleSectionType::Lifecycle)),
              diag::CapsuleSectionOwner::Shared);
    EXPECT_EQ(diag::capsuleSectionOwnerFromType(
                  static_cast<std::uint16_t>(diag::CapsuleSectionType::Reserved)),
              diag::CapsuleSectionOwner::Reserved);
    EXPECT_EQ(diag::capsuleSectionOwnerFromType(0x1234U), diag::CapsuleSectionOwner::Unknown);
}

TEST(DiagCapsule, EncodesHeaderAndSectionTableLittleEndian)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    buffer[40] = 0xA1U;
    buffer[41] = 0xB2U;
    buffer[42] = 0xC3U;
    buffer[43] = 0xD4U;

    const diag::CapsuleDescriptor   descriptor = makeOneSectionDescriptor();
    const diag::CapsuleEncodeResult result =
        diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor);

    ASSERT_EQ(result.result, diag::Result::Ok);
    EXPECT_EQ(result.encodedLength, kCapsuleBytes);
    EXPECT_EQ(buffer[0], 'D');
    EXPECT_EQ(buffer[1], 'G');
    EXPECT_EQ(buffer[2], 'C');
    EXPECT_EQ(buffer[3], 'P');
    EXPECT_EQ(buffer[4], 1U);
    EXPECT_EQ(buffer[5], 0U);
    EXPECT_EQ(buffer[8], kCapsuleBytes);
    EXPECT_EQ(buffer[9], 0U);
    EXPECT_EQ(buffer[12], 7U);
    EXPECT_EQ(buffer[16], 1U);
    EXPECT_EQ(buffer[24], static_cast<std::uint8_t>(diag::CapsuleSectionType::ApplicationDtc));
    EXPECT_EQ(buffer[28], descriptor.sections[0].offset);
}

TEST(DiagCapsule, DecodesValidCapsule)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    buffer[40] = 0x11U;
    buffer[41] = 0x22U;
    buffer[42] = 0x33U;
    buffer[43] = 0x44U;

    const diag::CapsuleDescriptor descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    const diag::ResultValue<diag::CapsuleDescriptor> decoded =
        diag::decodeCapsule(buffer.data(), buffer.size());

    ASSERT_TRUE(decoded.hasValue());
    EXPECT_EQ(decoded.value().schemaVersion, diag::kCapsuleSchemaVersion);
    EXPECT_EQ(decoded.value().sectionCount, 1U);
    EXPECT_EQ(decoded.value().totalLength, kCapsuleBytes);
    EXPECT_EQ(decoded.value().generation, 7U);
    EXPECT_EQ(decoded.value().sections[0].type,
              static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc));
    EXPECT_EQ(decoded.value().sections[0].version, 1U);
    EXPECT_EQ(decoded.value().sections[0].offset, descriptor.sections[0].offset);
    EXPECT_EQ(decoded.value().sections[0].length, 16U);
    EXPECT_EQ(decoded.value().sections[0].usedLength, 4U);
}

TEST(DiagCapsule, FindsKnownSectionByTypeAndOwner)
{
    const diag::CapsuleDescriptor descriptor = makeOneSectionDescriptor();

    const diag::ResultValue<diag::CapsuleSection> byType = diag::findCapsuleSectionByType(
        descriptor, static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc));
    ASSERT_TRUE(byType.hasValue());
    EXPECT_EQ(byType.value().usedLength, 4U);

    const diag::ResultValue<diag::CapsuleSection> byOwner =
        diag::findCapsuleSectionByOwner(descriptor, diag::CapsuleSectionOwner::Application);
    ASSERT_TRUE(byOwner.hasValue());
    EXPECT_EQ(byOwner.value().type,
              static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc));
}

TEST(DiagCapsule, ReportsMissingSections)
{
    const diag::CapsuleDescriptor descriptor = makeOneSectionDescriptor();

    EXPECT_EQ(diag::findCapsuleSectionByType(
                  descriptor, static_cast<std::uint16_t>(diag::CapsuleSectionType::BootloaderDtc))
                  .result(),
              diag::Result::NotFound);
    EXPECT_EQ(
        diag::findCapsuleSectionByOwner(descriptor, diag::CapsuleSectionOwner::Bootloader).result(),
        diag::Result::NotFound);
}

TEST(DiagCapsule, ValidatesDecodedSectionBounds)
{
    diag::CapsuleDescriptor descriptor = makeOneSectionDescriptor();

    EXPECT_EQ(diag::validateCapsuleSectionBounds(descriptor, descriptor.sections[0]),
              diag::Result::Ok);

    descriptor.sections[0].usedLength = descriptor.sections[0].length + 1U;
    EXPECT_EQ(diag::validateCapsuleSectionBounds(descriptor, descriptor.sections[0]),
              diag::Result::CorruptData);
}

TEST(DiagCapsule, CopiesKnownSectionPayloadIntoCallerBuffer)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    buffer[40] = 0x11U;
    buffer[41] = 0x22U;
    buffer[42] = 0x33U;
    buffer[43] = 0x44U;

    const diag::CapsuleDescriptor descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    const diag::ResultValue<diag::CapsuleDescriptor> decoded =
        diag::decodeCapsule(buffer.data(), buffer.size());
    ASSERT_TRUE(decoded.hasValue());

    std::array<std::uint8_t, 4U>         payload{};
    const diag::CapsulePayloadCopyResult result = diag::copyCapsuleSectionPayloadByType(
        buffer.data(), buffer.size(), decoded.value(),
        static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc), payload.data(),
        payload.size());

    ASSERT_EQ(result.result, diag::Result::Ok);
    EXPECT_EQ(result.copiedLength, 4U);
    EXPECT_EQ(payload[0], 0x11U);
    EXPECT_EQ(payload[1], 0x22U);
    EXPECT_EQ(payload[2], 0x33U);
    EXPECT_EQ(payload[3], 0x44U);
}

TEST(DiagCapsule, RejectsPayloadCopyWhenCallerBufferIsTooSmall)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    const diag::CapsuleDescriptor           descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    std::array<std::uint8_t, 3U>         payload{};
    const diag::CapsulePayloadCopyResult result = diag::copyCapsuleSectionPayloadByType(
        buffer.data(), buffer.size(), descriptor,
        static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc), payload.data(),
        payload.size());

    EXPECT_EQ(result.result, diag::Result::Capacity);
    EXPECT_EQ(result.copiedLength, 0U);
}

TEST(DiagCapsule, RejectsCorruptCapsuleMetadata)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    const diag::CapsuleDescriptor           descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    buffer[4] = 2U;
    buffer[5] = 0U;
    EXPECT_EQ(diag::decodeCapsule(buffer.data(), buffer.size()).result(),
              diag::Result::CorruptData);

    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);
    buffer[16] = static_cast<std::uint8_t>(diag::kCapsuleMaxSections + 1U);
    buffer[17] = 0U;
    EXPECT_EQ(diag::decodeCapsule(buffer.data(), buffer.size()).result(),
              diag::Result::CorruptData);
}

TEST(DiagCapsule, RejectsSectionLengthPastCapsuleEnd)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    const diag::CapsuleDescriptor           descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    buffer[28] = 120U;
    buffer[29] = 0U;
    buffer[30] = 0U;
    buffer[31] = 0U;
    buffer[32] = 16U;
    buffer[33] = 0U;
    buffer[34] = 0U;
    buffer[35] = 0U;

    EXPECT_EQ(diag::decodeCapsule(buffer.data(), buffer.size()).result(),
              diag::Result::CorruptData);
}

TEST(DiagCapsule, RejectsInvalidCrc)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    const diag::CapsuleDescriptor           descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    buffer[40] ^= 0x55U;

    EXPECT_EQ(diag::decodeCapsule(buffer.data(), buffer.size()).result(),
              diag::Result::CorruptData);
}

TEST(DiagCapsule, EncodeRejectsInvalidDescriptor)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    diag::CapsuleDescriptor                 descriptor = makeOneSectionDescriptor();

    descriptor.sectionCount = static_cast<std::uint16_t>(diag::kCapsuleMaxSections + 1U);
    EXPECT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Capacity);

    descriptor = makeOneSectionDescriptor();
    descriptor.sections[0].usedLength = descriptor.sections[0].length + 1U;
    EXPECT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::InvalidArgument);

    EXPECT_EQ(diag::encodeCapsuleV1(nullptr, buffer.size(), descriptor).result,
              diag::Result::InvalidArgument);
}

TEST(DiagCapsule, PayloadCopyRejectsInvalidArgumentsAndShortCapsule)
{
    std::array<std::uint8_t, kCapsuleBytes> buffer{};
    const diag::CapsuleDescriptor           descriptor = makeOneSectionDescriptor();
    ASSERT_EQ(diag::encodeCapsuleV1(buffer.data(), buffer.size(), descriptor).result,
              diag::Result::Ok);

    std::array<std::uint8_t, 4U> payload{};
    EXPECT_EQ(diag::copyCapsuleSectionPayload(nullptr, buffer.size(), descriptor,
                                              descriptor.sections[0], payload.data(),
                                              payload.size())
                  .result,
              diag::Result::InvalidArgument);
    EXPECT_EQ(diag::copyCapsuleSectionPayload(buffer.data(), buffer.size(), descriptor,
                                              descriptor.sections[0], nullptr, payload.size())
                  .result,
              diag::Result::InvalidArgument);
    EXPECT_EQ(diag::copyCapsuleSectionPayloadByType(
                  buffer.data(), descriptor.totalLength - 1U, descriptor,
                  static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc),
                  payload.data(), payload.size())
                  .result,
              diag::Result::CorruptData);
}
