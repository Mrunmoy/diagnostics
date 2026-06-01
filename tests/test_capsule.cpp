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
