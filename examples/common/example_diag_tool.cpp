#include "example_diag_tool.hpp"

#include <limits>

namespace example_diag
{

void writeU32Le(std::uint8_t *const buffer, const std::uint32_t value) noexcept
{
    buffer[0] = static_cast<std::uint8_t>(value & 0xFFU);
    buffer[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    buffer[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    buffer[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::uint32_t readU32Le(const std::uint8_t *const buffer) noexcept
{
    return static_cast<std::uint32_t>(buffer[0]) | (static_cast<std::uint32_t>(buffer[1]) << 8U) |
           (static_cast<std::uint32_t>(buffer[2]) << 16U) |
           (static_cast<std::uint32_t>(buffer[3]) << 24U);
}

namespace
{

#if DIAG_FEATURE_IDENTITY
[[nodiscard]] diag::Result appendIdentity(const Device &device, Frame &response) noexcept
{
    const diag::ResultValue<diag::Identity> identity = device.context->identity();
    if (!identity.hasValue())
    {
        return identity.result();
    }

    response.bytes[0] = static_cast<std::uint8_t>(diag::Result::Ok);
    response.bytes[1] = static_cast<std::uint8_t>(identity.value().ecosystem.value & 0xFFU);
    response.bytes[2] = static_cast<std::uint8_t>((identity.value().ecosystem.value >> 8U) & 0xFFU);
    response.bytes[3] = static_cast<std::uint8_t>(identity.value().product.value & 0xFFU);
    response.bytes[4] = static_cast<std::uint8_t>((identity.value().product.value >> 8U) & 0xFFU);
    response.bytes[5] = static_cast<std::uint8_t>(identity.value().deviceType.value & 0xFFU);
    response.bytes[6] =
        static_cast<std::uint8_t>((identity.value().deviceType.value >> 8U) & 0xFFU);
    response.bytes[7] = identity.value().instance.value;
    response.bytes[8] = identity.value().firmwareStage.value;
    response.bytes[9] = identity.value().firmwareComponent.value;
    response.bytes[10] = identity.value().reserved;
    response.size = 11U;
    return diag::Result::Ok;
}
#endif

#if DIAG_FEATURE_DTC
[[nodiscard]] diag::Result appendDtcList(const Device &device, Frame &response) noexcept
{
    diag::DtcRecord    records[kMaxDtcSnapshotCount]{};
    std::size_t        count = 0U;
    const diag::Result result = device.context->listDtcs(records, kMaxDtcSnapshotCount, count);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (count > (std::numeric_limits<std::uint8_t>::max)() ||
        (2U + (count * kDtcWireSize)) > sizeof(response.bytes))
    {
        return diag::Result::Capacity;
    }

    response.bytes[0] = static_cast<std::uint8_t>(diag::Result::Ok);
    response.bytes[1] = static_cast<std::uint8_t>(count);
    response.size = 2U + (count * kDtcWireSize);

    for (std::size_t index = 0U; index < count; ++index)
    {
        std::uint8_t *const record = &response.bytes[2U + (index * kDtcWireSize)];
        writeU32Le(&record[0], records[index].id.value);
        record[4] = records[index].status;
        record[5] = static_cast<std::uint8_t>(records[index].severity);
        writeU32Le(&record[6], records[index].occurrenceCount);
    }

    return diag::Result::Ok;
}

[[nodiscard]] diag::Result clearDtc(const Device &device, const Frame &request,
                                    Frame &response) noexcept
{
    if (request.size != 5U)
    {
        return diag::Result::InvalidArgument;
    }

    const diag::DtcId id{readU32Le(&request.bytes[1])};
    diag::Result      result = device.context->clearDtc(id);
#if DIAG_FEATURE_STORAGE && DIAG_FEATURE_CAPSULE
    if (result == diag::Result::Ok && device.persistedSize != nullptr)
    {
        result = device.context->savePersistent();
        if (result == diag::Result::Ok && device.persistedSize(device.user) == 0U)
        {
            result = diag::Result::CorruptData;
        }
    }
#endif

    response.bytes[0] = static_cast<std::uint8_t>(result);
    response.size = 1U;
    return result;
}
#endif

} // namespace

diag::Result handleRequest(const Device &device, const Frame &request, Frame &response) noexcept
{
    response.size = 0U;
    if (device.context == nullptr || request.size == 0U)
    {
        return diag::Result::InvalidArgument;
    }

    diag::Result result = diag::Result::Ok;

    switch (static_cast<Service>(request.bytes[0]))
    {
    case Service::ReadIdentity:
#if DIAG_FEATURE_IDENTITY
        result = appendIdentity(device, response);
#else
        result = diag::Result::NotSupported;
#endif
        break;

    case Service::ListDtcs:
#if DIAG_FEATURE_DTC
        result = appendDtcList(device, response);
#else
        result = diag::Result::NotSupported;
#endif
        break;

    case Service::ClearDtc:
#if DIAG_FEATURE_DTC
        result = clearDtc(device, request, response);
#else
        result = diag::Result::NotSupported;
#endif
        break;

    default:
        result = diag::Result::NotSupported;
        break;
    }

    if (result != diag::Result::Ok && response.size == 0U)
    {
        response.bytes[0] = static_cast<std::uint8_t>(result);
        response.size = 1U;
    }

    return result;
}

} // namespace example_diag
