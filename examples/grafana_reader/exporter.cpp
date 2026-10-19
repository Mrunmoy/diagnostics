#include "exporter.hpp"

#include <iostream>

namespace grafana_reader
{
namespace
{

[[nodiscard]] unsigned statusBitIsSet(const std::uint8_t status, const diag::DtcStatus mask)
{
    return ((status & static_cast<diag::DtcStatusFlags>(mask)) != 0U) ? 1U : 0U;
}

[[nodiscard]] diag::Result validateSnapshot(const example_diag::ToolSnapshot &snapshot) noexcept
{
    if (snapshot.dtcCount > example_diag::kMaxDtcSnapshotCount)
    {
        return diag::Result::Capacity;
    }

    return diag::Result::Ok;
}

} // namespace

diag::Result exportPrometheus(std::ostream &stream, const example_diag::ToolSnapshot &snapshot)
{
    const diag::Result result = validateSnapshot(snapshot);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    stream << "# HELP diag_tester_up Diagnostic tester scrape health.\n";
    stream << "# TYPE diag_tester_up gauge\n";
    stream << "diag_tester_up 1\n";
    stream << "# HELP diag_device_identity_info Compact diagnostic identity.\n";
    stream << "# TYPE diag_device_identity_info gauge\n";
    stream << "diag_device_identity_info{ecosystem_id=\"" << snapshot.identity.ecosystemId
           << "\",product_id=\"" << snapshot.identity.productId << "\",device_type=\""
           << snapshot.identity.deviceType << "\",device_instance=\""
           << static_cast<unsigned>(snapshot.identity.deviceInstance) << "\",stage=\""
           << static_cast<unsigned>(snapshot.identity.firmwareStage) << "\",component=\""
           << static_cast<unsigned>(snapshot.identity.firmwareComponent) << "\"} 1\n";
    stream << "# HELP diag_dtc_registered_info Registered DTC identity and severity.\n";
    stream << "# TYPE diag_dtc_registered_info gauge\n";
    stream << "# HELP diag_dtc_status_info Dashboard-friendly DTC status labels.\n";
    stream << "# TYPE diag_dtc_status_info gauge\n";
    stream << "# HELP diag_dtc_active DTC test_failed status bit.\n";
    stream << "# TYPE diag_dtc_active gauge\n";
    stream << "# HELP diag_dtc_confirmed DTC confirmed status bit.\n";
    stream << "# TYPE diag_dtc_confirmed gauge\n";
    stream << "# HELP diag_dtc_occurrences DTC occurrence count in the current snapshot.\n";
    stream << "# TYPE diag_dtc_occurrences gauge\n";

    for (std::size_t index = 0U; index < snapshot.dtcCount; ++index)
    {
        const example_diag::DtcSnapshot &dtc = snapshot.dtcs[index];
        const unsigned active = statusBitIsSet(dtc.status, diag::DtcStatus::TestFailed);
        const unsigned confirmed = statusBitIsSet(dtc.status, diag::DtcStatus::Confirmed);

        stream << "diag_dtc_registered_info{dtc_id=\"0x" << std::hex << dtc.id.value << std::dec
               << "\",severity=\"" << static_cast<unsigned>(dtc.severity) << "\"} 1\n";
        stream << "diag_dtc_status_info{dtc_id=\"0x" << std::hex << dtc.id.value << std::dec
               << "\",status=\"0x" << std::hex << static_cast<unsigned>(dtc.status) << std::dec
               << "\",severity=\"" << static_cast<unsigned>(dtc.severity) << "\",active=\""
               << active << "\",confirmed=\"" << confirmed << "\"} 1\n";
        stream << "diag_dtc_active{dtc_id=\"0x" << std::hex << dtc.id.value << std::dec << "\"} "
               << active << "\n";
        stream << "diag_dtc_confirmed{dtc_id=\"0x" << std::hex << dtc.id.value << std::dec << "\"} "
               << confirmed << "\n";
        stream << "diag_dtc_occurrences{dtc_id=\"0x" << std::hex << dtc.id.value << std::dec
               << "\"} " << dtc.occurrenceCount << "\n";
    }

    stream << "# HELP diag_capsule_persisted_bytes Stored diagnostic capsule size.\n";
    stream << "# TYPE diag_capsule_persisted_bytes gauge\n";
    stream << "diag_capsule_persisted_bytes " << snapshot.persistedSize << "\n";
    return diag::Result::Ok;
}

diag::Result exportJson(std::ostream &stream, const example_diag::ToolSnapshot &snapshot)
{
    const diag::Result result = validateSnapshot(snapshot);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    stream << "{\n";
    stream << "  \"identity\": {\n";
    stream << "    \"ecosystem_id\": " << snapshot.identity.ecosystemId << ",\n";
    stream << "    \"product_id\": " << snapshot.identity.productId << ",\n";
    stream << "    \"device_type\": " << snapshot.identity.deviceType << ",\n";
    stream << "    \"instance\": " << static_cast<unsigned>(snapshot.identity.deviceInstance)
           << ",\n";
    stream << "    \"stage\": " << static_cast<unsigned>(snapshot.identity.firmwareStage) << ",\n";
    stream << "    \"component\": " << static_cast<unsigned>(snapshot.identity.firmwareComponent)
           << "\n";
    stream << "  },\n";
    stream << "  \"dtcs\": [\n";

    for (std::size_t index = 0U; index < snapshot.dtcCount; ++index)
    {
        const example_diag::DtcSnapshot &dtc = snapshot.dtcs[index];
        const char *const                suffix = ((index + 1U) == snapshot.dtcCount) ? "" : ",";
        stream << "    {\"id\": \"0x" << std::hex << dtc.id.value << std::dec
               << "\", \"status\": " << static_cast<unsigned>(dtc.status)
               << ", \"severity\": " << static_cast<unsigned>(dtc.severity)
               << ", \"occurrences\": " << dtc.occurrenceCount << "}" << suffix << "\n";
    }

    stream << "  ],\n";
    stream << "  \"persisted_capsule_bytes\": " << snapshot.persistedSize << "\n";
    stream << "}\n";
    return diag::Result::Ok;
}

} // namespace grafana_reader
