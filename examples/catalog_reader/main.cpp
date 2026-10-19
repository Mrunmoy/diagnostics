#include "device.hpp"
#include "example_diag_tool.hpp"

#include "diag/diag.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>

namespace
{

struct CatalogIdentityEntry
{
    std::uint16_t ecosystemId;
    std::uint16_t productId;
    std::uint16_t deviceType;
    std::uint8_t  firmwareStage;
    std::uint8_t  firmwareComponent;
    const char   *catalogVersion;
    const char   *productName;
    const char   *deviceRole;
};

struct CatalogDtcEntry
{
    std::uint16_t ecosystemId;
    std::uint16_t productId;
    std::uint16_t deviceType;
    std::uint8_t  firmwareStage;
    std::uint8_t  firmwareComponent;
    diag::DtcId   dtcId;
    const char   *name;
    const char   *meaning;
    const char   *serviceAction;
};

constexpr CatalogIdentityEntry kCatalogIdentities[] = {
    {7U, 42U, 3U, 1U, 2U, "example-catalog-2026.06", "Reference Control Node",
     "bench diagnostic target"},
};

constexpr CatalogDtcEntry kCatalogDtcs[] = {
    {7U, 42U, 3U, 1U, 2U, diag::DtcId{0x030101U}, "Supply brownout detected",
     "The device observed a failed supply monitor during the current operation cycle.",
     "Check input supply range, connector retention, and brownout reset history."},
    {7U, 42U, 3U, 1U, 2U, diag::DtcId{0x030102U}, "Configuration fallback active",
     "The device is running with a safe default configuration after validation failed.",
     "Read configuration source, verify checksum, and reload the product profile."},
};

[[nodiscard]] bool statusMaskIsSet(const std::uint8_t status, const diag::DtcStatus mask) noexcept
{
    return (status & static_cast<diag::DtcStatusFlags>(mask)) != 0U;
}

[[nodiscard]] const char *severityName(const std::uint8_t severity) noexcept
{
    switch (static_cast<diag::DtcSeverity>(severity))
    {
    case diag::DtcSeverity::Info:
        return "info";
    case diag::DtcSeverity::Warning:
        return "warning";
    case diag::DtcSeverity::Error:
        return "error";
    case diag::DtcSeverity::Critical:
        return "critical";
    default:
        return "unknown";
    }
}

[[nodiscard]] const CatalogIdentityEntry *
findIdentityEntry(const example_diag::IdentitySnapshot &identity) noexcept
{
    for (const CatalogIdentityEntry &entry : kCatalogIdentities)
    {
        if (entry.ecosystemId == identity.ecosystemId && entry.productId == identity.productId &&
            entry.deviceType == identity.deviceType &&
            entry.firmwareStage == identity.firmwareStage &&
            entry.firmwareComponent == identity.firmwareComponent)
        {
            return &entry;
        }
    }

    return nullptr;
}

[[nodiscard]] const CatalogDtcEntry *findDtcEntry(const example_diag::IdentitySnapshot &identity,
                                                  const diag::DtcId dtcId) noexcept
{
    for (const CatalogDtcEntry &entry : kCatalogDtcs)
    {
        if (entry.ecosystemId == identity.ecosystemId && entry.productId == identity.productId &&
            entry.deviceType == identity.deviceType &&
            entry.firmwareStage == identity.firmwareStage &&
            entry.firmwareComponent == identity.firmwareComponent && entry.dtcId == dtcId)
        {
            return &entry;
        }
    }

    return nullptr;
}

void printStatusSummary(const std::uint8_t status)
{
    std::cout << "status=0x" << std::hex << static_cast<int>(status) << std::dec
              << " active=" << (statusMaskIsSet(status, diag::DtcStatus::TestFailed) ? "yes" : "no")
              << " pending=" << (statusMaskIsSet(status, diag::DtcStatus::Pending) ? "yes" : "no")
              << " confirmed="
              << (statusMaskIsSet(status, diag::DtcStatus::Confirmed) ? "yes" : "no")
              << " failed_since_clear="
              << (statusMaskIsSet(status, diag::DtcStatus::TestFailedSinceClear) ? "yes" : "no")
              << "\n";
}

[[nodiscard]] diag::Result printCatalogView(const example_diag::ToolSnapshot &snapshot)
{
    const CatalogIdentityEntry *const identityEntry = findIdentityEntry(snapshot.identity);
    if (identityEntry == nullptr)
    {
        std::cerr << "catalog_reader: no catalog for identity ecosystem="
                  << snapshot.identity.ecosystemId << " product=" << snapshot.identity.productId
                  << " type=" << snapshot.identity.deviceType << "\n";
        return diag::Result::NotFound;
    }

    std::cout << "catalog_reader: catalog=" << identityEntry->catalogVersion << "\n";
    std::cout << "catalog_reader: product=" << identityEntry->productName
              << " role=" << identityEntry->deviceRole
              << " instance=" << static_cast<int>(snapshot.identity.deviceInstance) << "\n";
    std::cout << "catalog_reader: compact identity ecosystem=" << snapshot.identity.ecosystemId
              << " product=" << snapshot.identity.productId
              << " type=" << snapshot.identity.deviceType
              << " stage=" << static_cast<int>(snapshot.identity.firmwareStage)
              << " component=" << static_cast<int>(snapshot.identity.firmwareComponent) << "\n";
    std::cout << "catalog_reader: DTC count=" << snapshot.dtcCount
              << " persisted_capsule_bytes=" << snapshot.persistedSize << "\n";

    std::size_t describedCount = 0U;
    for (std::size_t index = 0U; index < snapshot.dtcCount; ++index)
    {
        const example_diag::DtcSnapshot &dtc = snapshot.dtcs[index];
        const CatalogDtcEntry *const     entry = findDtcEntry(snapshot.identity, dtc.id);

        std::cout << "catalog_reader: DTC 0x" << std::hex << dtc.id.value << std::dec
                  << " severity=" << severityName(dtc.severity)
                  << " occurrences=" << dtc.occurrenceCount << " ";
        printStatusSummary(dtc.status);

        if (entry == nullptr)
        {
            std::cout << "  name: unknown catalog entry\n";
            continue;
        }

        ++describedCount;
        std::cout << "  name: " << entry->name << "\n";
        std::cout << "  meaning: " << entry->meaning << "\n";
        std::cout << "  action: " << entry->serviceAction << "\n";
    }

    if (describedCount != snapshot.dtcCount)
    {
        std::cerr << "catalog_reader: one or more DTCs did not have catalog entries\n";
        return diag::Result::NotFound;
    }

    return diag::Result::Ok;
}

} // namespace

int main()
{
    diagnostic_session::DiagnosticDevice device{};
    if (device.setup() != diag::Result::Ok)
    {
        std::cerr << "catalog_reader: failed to initialize diagnostic device\n";
        return 1;
    }

    const example_diag::Device endpoint{"catalog_reader", &device.context(),
                                        diagnostic_session::persistedSizeCallback, &device};
    example_diag::ToolSnapshot snapshot{};
    if (example_diag::collectSnapshot(endpoint, snapshot) != diag::Result::Ok)
    {
        std::cerr << "catalog_reader: failed to collect diagnostic snapshot\n";
        return 1;
    }

    if (printCatalogView(snapshot) != diag::Result::Ok)
    {
        return 1;
    }

    return 0;
}
