#include "diag/diag.hpp"

#include <array>
#include <cstdint>
#include <iostream>

namespace
{

int fail(const char *const message)
{
    std::cerr << "capsule_example: " << message << '\n';
    return 1;
}

} // namespace

int main()
{
    constexpr std::size_t kCapsuleBytes = 96U;

    std::array<std::uint8_t, kCapsuleBytes> capsule{};
    capsule[40] = 0xDEU;
    capsule[41] = 0xADU;
    capsule[42] = 0xBEU;
    capsule[43] = 0xEFU;

    diag::CapsuleDescriptor descriptor{};
    descriptor.sectionCount = 1U;
    descriptor.totalLength = capsule.size();
    descriptor.generation = 3U;
    descriptor.sections[0].type =
        static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc);
    descriptor.sections[0].version = 1U;
    descriptor.sections[0].offset = diag::kCapsuleHeaderSize + diag::kCapsuleSectionEntrySize;
    descriptor.sections[0].length = 16U;
    descriptor.sections[0].usedLength = 4U;

    const diag::CapsuleEncodeResult encoded =
        diag::encodeCapsuleV1(capsule.data(), capsule.size(), descriptor);
    if (encoded.result != diag::Result::Ok)
    {
        return fail("failed to encode capsule");
    }

    const diag::ResultValue<diag::CapsuleDescriptor> decoded =
        diag::decodeCapsule(capsule.data(), capsule.size());
    if (!decoded.hasValue())
    {
        return fail("failed to decode capsule");
    }

    std::array<std::uint8_t, 4U>         payload{};
    const diag::CapsulePayloadCopyResult copied = diag::copyCapsuleSectionPayloadByType(
        capsule.data(), capsule.size(), decoded.value(),
        static_cast<std::uint16_t>(diag::CapsuleSectionType::ApplicationDtc), payload.data(),
        payload.size());
    if (copied.result != diag::Result::Ok)
    {
        return fail("failed to copy capsule payload");
    }

    std::cout << "capsule_example: generation=" << decoded.value().generation
              << " sections=" << decoded.value().sectionCount
              << " payload_bytes=" << copied.copiedLength << '\n';

    if (payload[0] != 0xDEU || payload[1] != 0xADU || payload[2] != 0xBEU || payload[3] != 0xEFU)
    {
        return fail("unexpected payload contents");
    }

    return 0;
}
