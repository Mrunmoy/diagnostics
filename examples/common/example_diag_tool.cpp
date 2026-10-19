#include "example_diag_tool.hpp"

#include <iomanip>
#include <iostream>
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

namespace
{

[[nodiscard]] diag::Result exchangeChecked(const Device &device, const Frame &request,
                                           Frame &response, std::ostream &err)
{
    const diag::Result result = handleRequest(device, request, response);
    if (result != diag::Result::Ok)
    {
        err << device.name << " tool: request 0x" << std::hex << static_cast<int>(request.bytes[0])
            << std::dec << " failed\n";
        return result;
    }

    if (response.size == 0U)
    {
        err << device.name << " tool: empty response\n";
        return diag::Result::CorruptData;
    }

    const auto status = static_cast<diag::Result>(response.bytes[0]);
    if (status != diag::Result::Ok)
    {
        err << device.name << " tool: device rejected request 0x" << std::hex
            << static_cast<int>(request.bytes[0]) << std::dec << "\n";
        return status;
    }

    return diag::Result::Ok;
}

[[nodiscard]] diag::Result collectIdentity(const Device     &device,
                                           IdentitySnapshot &outIdentity) noexcept
{
    Frame request{};
    Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(Service::ReadIdentity);
    request.size = 1U;

    const diag::Result result = handleRequest(device, request, response);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size != 11U || static_cast<diag::Result>(response.bytes[0]) != diag::Result::Ok)
    {
        return diag::Result::CorruptData;
    }

    outIdentity.ecosystemId =
        static_cast<std::uint16_t>(response.bytes[1] | (response.bytes[2] << 8U));
    outIdentity.productId =
        static_cast<std::uint16_t>(response.bytes[3] | (response.bytes[4] << 8U));
    outIdentity.deviceType =
        static_cast<std::uint16_t>(response.bytes[5] | (response.bytes[6] << 8U));
    outIdentity.deviceInstance = response.bytes[7];
    outIdentity.firmwareStage = response.bytes[8];
    outIdentity.firmwareComponent = response.bytes[9];
    return diag::Result::Ok;
}

[[nodiscard]] diag::Result collectDtcs(const Device &device, ToolSnapshot &outSnapshot) noexcept
{
    Frame request{};
    Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(Service::ListDtcs);
    request.size = 1U;

    const diag::Result result = handleRequest(device, request, response);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size < 2U || static_cast<diag::Result>(response.bytes[0]) != diag::Result::Ok)
    {
        return diag::Result::CorruptData;
    }

    const std::size_t count = response.bytes[1];
    if (count > kMaxDtcSnapshotCount || response.size != (2U + (count * kDtcWireSize)))
    {
        return diag::Result::CorruptData;
    }

    outSnapshot.dtcCount = count;
    for (std::size_t index = 0U; index < count; ++index)
    {
        const std::uint8_t *const record = &response.bytes[2U + (index * kDtcWireSize)];
        outSnapshot.dtcs[index].id = diag::DtcId{readU32Le(&record[0])};
        outSnapshot.dtcs[index].status = record[4];
        outSnapshot.dtcs[index].severity = record[5];
        outSnapshot.dtcs[index].occurrenceCount = readU32Le(&record[6]);
    }

    return diag::Result::Ok;
}

[[nodiscard]] diag::Result readIdentityCli(const Device &device, std::ostream &out,
                                           std::ostream &err)
{
    Frame request{};
    Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(Service::ReadIdentity);
    request.size = 1U;

    const diag::Result result = exchangeChecked(device, request, response, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size != 11U)
    {
        err << device.name << " tool: malformed identity response\n";
        return diag::Result::CorruptData;
    }

    const std::uint16_t ecosystem =
        static_cast<std::uint16_t>(response.bytes[1] | (response.bytes[2] << 8U));
    const std::uint16_t product =
        static_cast<std::uint16_t>(response.bytes[3] | (response.bytes[4] << 8U));
    const std::uint16_t deviceType =
        static_cast<std::uint16_t>(response.bytes[5] | (response.bytes[6] << 8U));

    out << device.name << " tool: identity ecosystem=" << ecosystem << " product=" << product
        << " type=" << deviceType << " instance=" << static_cast<int>(response.bytes[7])
        << " stage=" << static_cast<int>(response.bytes[8])
        << " component=" << static_cast<int>(response.bytes[9]) << "\n";
    return diag::Result::Ok;
}

[[nodiscard]] diag::Result listDtcsCli(const Device &device, const diag::DtcId clearDtcId,
                                       std::uint8_t &outClearStatus, std::ostream &out,
                                       std::ostream &err)
{
    Frame request{};
    Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(Service::ListDtcs);
    request.size = 1U;

    const diag::Result result = exchangeChecked(device, request, response, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size < 2U)
    {
        err << device.name << " tool: malformed DTC list response\n";
        return diag::Result::CorruptData;
    }

    const std::size_t count = response.bytes[1];
    if (response.size != (2U + (count * kDtcWireSize)))
    {
        err << device.name << " tool: DTC list length mismatch\n";
        return diag::Result::CorruptData;
    }

    out << device.name << " tool: DTC count=" << count << "\n";
    bool foundClearTarget = false;
    for (std::size_t index = 0U; index < count; ++index)
    {
        const std::uint8_t *const record = &response.bytes[2U + (index * kDtcWireSize)];
        const diag::DtcId         id{readU32Le(&record[0])};
        const std::uint8_t        status = record[4];
        const std::uint8_t        severity = record[5];
        const std::uint32_t       occurrences = readU32Le(&record[6]);

        out << device.name << " tool: DTC 0x" << std::hex << std::setw(6) << std::setfill('0')
            << id.value << std::setfill(' ') << std::dec << " status=0x" << std::hex << std::setw(2)
            << std::setfill('0') << static_cast<int>(status) << std::setfill(' ') << std::dec
            << " severity=" << static_cast<int>(severity) << " occurrences=" << occurrences << "\n";

        if (clearDtcId.value != 0U && id == clearDtcId)
        {
            outClearStatus = status;
            foundClearTarget = true;
        }
    }

    if (clearDtcId.value != 0U && !foundClearTarget)
    {
        err << device.name << " tool: DTC 0x" << std::hex << std::setw(6) << std::setfill('0')
            << clearDtcId.value << std::setfill(' ') << std::dec << " was not reported\n";
        return diag::Result::NotFound;
    }

    return diag::Result::Ok;
}

[[nodiscard]] diag::Result clearDtcCli(const Device &device, const diag::DtcId clearDtcId,
                                       std::ostream &out, std::ostream &err)
{
    Frame request{};
    Frame response{};
    request.bytes[0] = static_cast<std::uint8_t>(Service::ClearDtc);
    writeU32Le(&request.bytes[1], clearDtcId.value);
    request.size = 5U;

    const diag::Result result = exchangeChecked(device, request, response, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (response.size != 1U)
    {
        err << device.name << " tool: malformed clear response\n";
        return diag::Result::CorruptData;
    }

    out << device.name << " tool: cleared DTC 0x" << std::hex << std::setw(6) << std::setfill('0')
        << clearDtcId.value << std::setfill(' ') << std::dec << "\n";
    return diag::Result::Ok;
}

} // namespace

diag::Result collectSnapshot(const Device &device, ToolSnapshot &outSnapshot) noexcept
{
    if (device.name == nullptr || device.context == nullptr)
    {
        return diag::Result::InvalidArgument;
    }

    outSnapshot = ToolSnapshot{};

    const diag::Result identityResult = collectIdentity(device, outSnapshot.identity);
    if (identityResult != diag::Result::Ok)
    {
        return identityResult;
    }

    const diag::Result dtcResult = collectDtcs(device, outSnapshot);
    if (dtcResult != diag::Result::Ok)
    {
        return dtcResult;
    }

    if (device.persistedSize != nullptr)
    {
        outSnapshot.persistedSize = device.persistedSize(device.user);
    }

    return diag::Result::Ok;
}

diag::Result runCli(const Device &device, const diag::DtcId clearDtcId, std::ostream &out,
                    std::ostream &err)
{
    if (device.name == nullptr || device.context == nullptr)
    {
        return diag::Result::InvalidArgument;
    }

    out << device.name << " tool: opening diagnostic session\n";

    diag::Result result = readIdentityCli(device, out, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    std::uint8_t statusBeforeClear = 0xFFU;
    std::uint8_t statusAfterClear = 0xFFU;
    result = listDtcsCli(device, clearDtcId, statusBeforeClear, out, err);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (clearDtcId.value != 0U)
    {
        result = clearDtcCli(device, clearDtcId, out, err);
        if (result != diag::Result::Ok)
        {
            return result;
        }

        result = listDtcsCli(device, clearDtcId, statusAfterClear, out, err);
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (device.persistedSize != nullptr)
    {
        out << device.name << " tool: persisted capsule bytes=" << device.persistedSize(device.user)
            << "\n";
    }

    if (clearDtcId.value != 0U && statusBeforeClear == 0U)
    {
        err << device.name
            << " tool: expected clear target to have diagnostic status before clear\n";
        return diag::Result::CorruptData;
    }

    if (clearDtcId.value != 0U && statusAfterClear != 0U)
    {
        err << device.name << " tool: expected clear target status to become zero\n";
        return diag::Result::CorruptData;
    }

    out << device.name << " tool: diagnostic session complete\n";
    return diag::Result::Ok;
}

} // namespace example_diag
