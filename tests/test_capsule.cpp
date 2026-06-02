#include <array>
#include <cstdint>
#include <cstring>

#include <gtest/gtest.h>

extern "C"
{
#include "diag/capsule.h"
}

namespace
{

constexpr std::size_t kCapsuleBytes = 128u;

struct diag_capsule_descriptor make_one_section_descriptor()
{
    struct diag_capsule_descriptor descriptor = {};
    descriptor.schema_version = DIAG_CAPSULE_SCHEMA_VERSION;
    descriptor.section_count = 1u;
    descriptor.total_length = kCapsuleBytes;
    descriptor.generation = 7u;
    descriptor.sections[0].type = DIAG_CAPSULE_SECTION_APPLICATION_DTC;
    descriptor.sections[0].version = 1u;
    descriptor.sections[0].offset = DIAG_CAPSULE_HEADER_SIZE + DIAG_CAPSULE_SECTION_ENTRY_SIZE;
    descriptor.sections[0].length = 16u;
    descriptor.sections[0].used_length = 4u;
    return descriptor;
}

TEST(DiagCapsule, EncodesHeaderAndSectionTableLittleEndian)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    buffer[40] = 0xA1u;
    buffer[41] = 0xB2u;
    buffer[42] = 0xC3u;
    buffer[43] = 0xD4u;

    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    std::size_t encoded_length = 0u;

    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, &encoded_length),
              DIAG_OK);

    EXPECT_EQ(encoded_length, kCapsuleBytes);
    EXPECT_EQ(buffer[0], 'D');
    EXPECT_EQ(buffer[1], 'G');
    EXPECT_EQ(buffer[2], 'C');
    EXPECT_EQ(buffer[3], 'P');
    EXPECT_EQ(buffer[4], 1u);
    EXPECT_EQ(buffer[5], 0u);
    EXPECT_EQ(buffer[8], kCapsuleBytes);
    EXPECT_EQ(buffer[9], 0u);
    EXPECT_EQ(buffer[12], 7u);
    EXPECT_EQ(buffer[16], 1u);
    EXPECT_EQ(buffer[24], DIAG_CAPSULE_SECTION_APPLICATION_DTC);
    EXPECT_EQ(buffer[28], descriptor.sections[0].offset);
}

TEST(DiagCapsule, DecodesValidCapsule)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    buffer[40] = 0x11u;
    buffer[41] = 0x22u;
    buffer[42] = 0x33u;
    buffer[43] = 0x44u;

    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    struct diag_capsule_descriptor decoded = {};
    ASSERT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_OK);

    EXPECT_EQ(decoded.schema_version, DIAG_CAPSULE_SCHEMA_VERSION);
    EXPECT_EQ(decoded.section_count, 1u);
    EXPECT_EQ(decoded.total_length, kCapsuleBytes);
    EXPECT_EQ(decoded.generation, 7u);
    EXPECT_EQ(decoded.sections[0].type, DIAG_CAPSULE_SECTION_APPLICATION_DTC);
    EXPECT_EQ(decoded.sections[0].version, 1u);
    EXPECT_EQ(decoded.sections[0].offset, descriptor.sections[0].offset);
    EXPECT_EQ(decoded.sections[0].length, 16u);
    EXPECT_EQ(decoded.sections[0].used_length, 4u);
}

TEST(DiagCapsule, FindsKnownSectionByType)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    struct diag_capsule_descriptor decoded = {};
    ASSERT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_OK);

    const struct diag_capsule_section *section = nullptr;
    ASSERT_EQ(
        diag_capsule_find_section_by_type(&decoded, DIAG_CAPSULE_SECTION_APPLICATION_DTC, &section),
        DIAG_OK);

    ASSERT_NE(section, nullptr);
    EXPECT_EQ(section->type, DIAG_CAPSULE_SECTION_APPLICATION_DTC);
    EXPECT_EQ(section->used_length, 4u);
}

TEST(DiagCapsule, ReportsMissingSectionByType)
{
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    const struct diag_capsule_section *section = nullptr;

    EXPECT_EQ(diag_capsule_find_section_by_type(&descriptor, DIAG_CAPSULE_SECTION_BOOTLOADER_DTC,
                                                &section),
              DIAG_ERROR_NOT_FOUND);
    EXPECT_EQ(section, nullptr);
}

TEST(DiagCapsule, FindsKnownSectionByOwner)
{
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    const struct diag_capsule_section *section = nullptr;

    ASSERT_EQ(diag_capsule_find_section_by_owner(&descriptor,
                                                 DIAG_CAPSULE_SECTION_OWNER_APPLICATION, &section),
              DIAG_OK);

    ASSERT_NE(section, nullptr);
    EXPECT_EQ(section->type, DIAG_CAPSULE_SECTION_APPLICATION_DTC);
}

TEST(DiagCapsule, ValidatesDecodedSectionBounds)
{
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();

    EXPECT_EQ(diag_capsule_validate_section_bounds(&descriptor, &descriptor.sections[0]), DIAG_OK);

    descriptor.sections[0].used_length = descriptor.sections[0].length + 1u;
    EXPECT_EQ(diag_capsule_validate_section_bounds(&descriptor, &descriptor.sections[0]),
              DIAG_ERROR_CORRUPT_DATA);
}

TEST(DiagCapsule, CopiesKnownSectionPayloadIntoCallerBuffer)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    buffer[40] = 0x11u;
    buffer[41] = 0x22u;
    buffer[42] = 0x33u;
    buffer[43] = 0x44u;

    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    struct diag_capsule_descriptor decoded = {};
    ASSERT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_OK);

    std::array<uint8_t, 4u> payload = {};
    std::size_t copied = 0u;
    ASSERT_EQ(diag_capsule_copy_section_payload_by_type(buffer.data(), buffer.size(), &decoded,
                                                        DIAG_CAPSULE_SECTION_APPLICATION_DTC,
                                                        payload.data(), payload.size(), &copied),
              DIAG_OK);

    EXPECT_EQ(copied, 4u);
    EXPECT_EQ(payload[0], 0x11u);
    EXPECT_EQ(payload[1], 0x22u);
    EXPECT_EQ(payload[2], 0x33u);
    EXPECT_EQ(payload[3], 0x44u);
}

TEST(DiagCapsule, RejectsPayloadCopyWhenCallerBufferIsTooSmall)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    std::array<uint8_t, 3u> payload = {};
    std::size_t copied = 99u;
    EXPECT_EQ(diag_capsule_copy_section_payload_by_type(buffer.data(), buffer.size(), &descriptor,
                                                        DIAG_CAPSULE_SECTION_APPLICATION_DTC,
                                                        payload.data(), payload.size(), &copied),
              DIAG_ERROR_CAPACITY);
    EXPECT_EQ(copied, 0u);
}

TEST(DiagCapsule, HelperLookupRejectsOversizedDescriptorSectionCountBeforeIteration)
{
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    descriptor.section_count = DIAG_CAPSULE_MAX_SECTIONS + 1u;

    const struct diag_capsule_section *section = nullptr;
    EXPECT_EQ(diag_capsule_find_section_by_type(&descriptor, DIAG_CAPSULE_SECTION_APPLICATION_DTC,
                                                &section),
              DIAG_ERROR_CORRUPT_DATA);
    EXPECT_EQ(section, nullptr);
}

TEST(DiagCapsule, PayloadCopyRejectsShortCapsuleBuffer)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    std::array<uint8_t, 4u> payload = {};
    std::size_t copied = 99u;
    EXPECT_EQ(diag_capsule_copy_section_payload_by_type(
                  buffer.data(), descriptor.total_length - 1u, &descriptor,
                  DIAG_CAPSULE_SECTION_APPLICATION_DTC, payload.data(), payload.size(), &copied),
              DIAG_ERROR_CORRUPT_DATA);
    EXPECT_EQ(copied, 0u);
}

TEST(DiagCapsule, RejectsUnsupportedSchemaVersion)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    buffer[4] = 2u;
    buffer[5] = 0u;

    struct diag_capsule_descriptor decoded = {};
    EXPECT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_ERROR_CORRUPT_DATA);
}

TEST(DiagCapsule, RejectsOversizedSectionCountBeforeIteration)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    buffer[16] = static_cast<uint8_t>(DIAG_CAPSULE_MAX_SECTIONS + 1u);
    buffer[17] = 0u;

    struct diag_capsule_descriptor decoded = {};
    EXPECT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_ERROR_CORRUPT_DATA);
}

TEST(DiagCapsule, RejectsSectionLengthPastCapsuleEnd)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    buffer[28] = 120u;
    buffer[29] = 0u;
    buffer[30] = 0u;
    buffer[31] = 0u;
    buffer[32] = 16u;
    buffer[33] = 0u;
    buffer[34] = 0u;
    buffer[35] = 0u;

    struct diag_capsule_descriptor decoded = {};
    EXPECT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_ERROR_CORRUPT_DATA);
}

TEST(DiagCapsule, RejectsInvalidCrc)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    buffer[40] ^= 0x55u;

    struct diag_capsule_descriptor decoded = {};
    EXPECT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_ERROR_CORRUPT_DATA);
}

TEST(DiagCapsule, RejectsSectionMetadataChangedAfterEncode)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    ASSERT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr), DIAG_OK);

    buffer[24] = DIAG_CAPSULE_SECTION_BOOTLOADER_DTC;

    struct diag_capsule_descriptor decoded = {};
    EXPECT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), &decoded), DIAG_ERROR_CORRUPT_DATA);
}

TEST(DiagCapsule, EncodeRejectsDescriptorWithTooManySections)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    descriptor.section_count = DIAG_CAPSULE_MAX_SECTIONS + 1u;

    EXPECT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr),
              DIAG_ERROR_CAPACITY);
}

TEST(DiagCapsule, EncodeReportsStructurallyInvalidDescriptorAsInvalidArgument)
{
    // A descriptor whose sections fail structural validation is bad caller input,
    // not corrupt on-wire data. Encode must report DIAG_ERROR_INVALID_ARGUMENT so
    // callers can distinguish a bad request from a corrupt decode (which is the
    // only place DIAG_ERROR_CORRUPT_DATA is appropriate).
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();
    descriptor.sections[0].used_length = descriptor.sections[0].length + 1u;

    EXPECT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), &descriptor, nullptr),
              DIAG_ERROR_INVALID_ARGUMENT);
}

TEST(DiagCapsule, RejectsNullArguments)
{
    std::array<uint8_t, kCapsuleBytes> buffer = {};
    struct diag_capsule_descriptor descriptor = make_one_section_descriptor();

    EXPECT_EQ(diag_capsule_encode_v1(nullptr, buffer.size(), &descriptor, nullptr),
              DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_capsule_encode_v1(buffer.data(), buffer.size(), nullptr, nullptr),
              DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_capsule_decode(nullptr, buffer.size(), &descriptor),
              DIAG_ERROR_INVALID_ARGUMENT);
    EXPECT_EQ(diag_capsule_decode(buffer.data(), buffer.size(), nullptr),
              DIAG_ERROR_INVALID_ARGUMENT);
}

} // namespace
