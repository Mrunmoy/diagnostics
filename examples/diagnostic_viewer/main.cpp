#include "device.hpp"
#include "example_diag_tool.hpp"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace
{

void printUsage(const char *program)
{
    std::cerr << "usage: " << program << " [--json] [--scenario-step N] [--clear DTC_ID]\n";
}

[[nodiscard]] bool parseU32(const char *text, std::uint32_t &outValue)
{
    if (text == nullptr || *text == '\0' || *text == '-')
    {
        return false;
    }

    char *end = nullptr;
    int   base = 10;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
        base = 16;
    }

    errno = 0;
    const unsigned long value = std::strtoul(text, &end, base);
    if (errno == ERANGE || end == text || *end != '\0' || value > UINT32_MAX)
    {
        return false;
    }

    outValue = static_cast<std::uint32_t>(value);
    return true;
}

[[nodiscard]] unsigned statusBitIsSet(const std::uint8_t status, const diag::DtcStatus mask)
{
    return ((status & static_cast<diag::DtcStatusFlags>(mask)) != 0U) ? 1U : 0U;
}

[[nodiscard]] const char *severityName(const std::uint8_t severity)
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

[[nodiscard]] const char *dtcTitle(const diag::DtcId id)
{
    switch (id.value)
    {
    case 0x050101U:
        return "Heater rail over-temperature";
    case 0x050102U:
        return "Cooling fan speed degraded";
    case 0x050103U:
        return "Fieldbus heartbeat missed";
    default:
        return "Unknown diagnostic trouble code";
    }
}

void printJson(const example_diag::ToolSnapshot &snapshot, const std::uint32_t scenarioStep)
{
    unsigned activeCount = 0U;
    unsigned confirmedCount = 0U;
    for (std::size_t index = 0U; index < snapshot.dtcCount; ++index)
    {
        const example_diag::DtcSnapshot &dtc = snapshot.dtcs[index];
        activeCount += statusBitIsSet(dtc.status, diag::DtcStatus::TestFailed);
        confirmedCount += statusBitIsSet(dtc.status, diag::DtcStatus::Confirmed);
    }

    std::cout << "{\n";
    std::cout << "  \"scenario_step\": " << scenarioStep << ",\n";
    std::cout << "  \"summary\": {\n";
    std::cout << "    \"active_dtcs\": " << activeCount << ",\n";
    std::cout << "    \"confirmed_dtcs\": " << confirmedCount << ",\n";
    std::cout << "    \"stored_bytes\": " << snapshot.persistedSize << ",\n";
    std::cout << "    \"health\": \"" << (activeCount > 0U ? "service_required" : "nominal")
              << "\"\n";
    std::cout << "  },\n";
    std::cout << "  \"identity\": {\n";
    std::cout << "    \"ecosystem_id\": " << snapshot.identity.ecosystemId << ",\n";
    std::cout << "    \"product_id\": " << snapshot.identity.productId << ",\n";
    std::cout << "    \"device_type\": " << snapshot.identity.deviceType << ",\n";
    std::cout << "    \"device_instance\": "
              << static_cast<unsigned>(snapshot.identity.deviceInstance) << ",\n";
    std::cout << "    \"firmware_stage\": "
              << static_cast<unsigned>(snapshot.identity.firmwareStage) << ",\n";
    std::cout << "    \"firmware_component\": "
              << static_cast<unsigned>(snapshot.identity.firmwareComponent) << "\n";
    std::cout << "  },\n";
    std::cout << "  \"dtcs\": [\n";

    for (std::size_t index = 0U; index < snapshot.dtcCount; ++index)
    {
        const example_diag::DtcSnapshot &dtc = snapshot.dtcs[index];
        const char *const                suffix = ((index + 1U) == snapshot.dtcCount) ? "" : ",";
        const unsigned active = statusBitIsSet(dtc.status, diag::DtcStatus::TestFailed);
        const unsigned confirmed = statusBitIsSet(dtc.status, diag::DtcStatus::Confirmed);

        std::cout << "    {";
        std::cout << "\"id\":\"0x" << std::hex << dtc.id.value << std::dec << "\",";
        std::cout << "\"title\":\"" << dtcTitle(dtc.id) << "\",";
        std::cout << "\"severity\":\"" << severityName(dtc.severity) << "\",";
        std::cout << "\"severity_code\":" << static_cast<unsigned>(dtc.severity) << ",";
        std::cout << "\"status\":" << static_cast<unsigned>(dtc.status) << ",";
        std::cout << "\"active\":" << (active != 0U ? "true" : "false") << ",";
        std::cout << "\"confirmed\":" << (confirmed != 0U ? "true" : "false") << ",";
        std::cout << "\"occurrences\":" << dtc.occurrenceCount;
        std::cout << "}" << suffix << "\n";
    }

    std::cout << "  ]\n";
    std::cout << "}\n";
}

} // namespace

int main(int argc, char **argv)
{
    std::uint32_t scenarioStep = 0U;
    std::uint32_t clearDtcId = 0U;
    int           i = 1;

    while (i < argc)
    {
        if (std::strcmp(argv[i], "--json") == 0)
        {
            ++i;
        }
        else if (std::strcmp(argv[i], "--scenario-step") == 0 && (i + 1) < argc &&
                 parseU32(argv[i + 1], scenarioStep))
        {
            i += 2;
        }
        else if (std::strcmp(argv[i], "--clear") == 0 && (i + 1) < argc &&
                 parseU32(argv[i + 1], clearDtcId))
        {
            i += 2;
        }
        else
        {
            printUsage(argv[0]);
            return 1;
        }
    }

    diagnostic_viewer::ViewerDevice device{};
    if (device.setup() != diag::Result::Ok)
    {
        std::cerr << "diagnostic_viewer: device setup failed\n";
        return 1;
    }

    if (device.applyScenario(scenarioStep) != diag::Result::Ok)
    {
        std::cerr << "diagnostic_viewer: scenario failed\n";
        return 1;
    }

    if (clearDtcId != 0U && device.clearDtc(diag::DtcId{clearDtcId}) != diag::Result::Ok)
    {
        std::cerr << "diagnostic_viewer: clear failed\n";
        return 1;
    }

    const example_diag::Device endpoint{"diagnostic_viewer", &device.context(),
                                        diagnostic_viewer::persistedSizeCallback, &device};
    example_diag::ToolSnapshot snapshot{};
    if (example_diag::collectSnapshot(endpoint, snapshot) != diag::Result::Ok)
    {
        std::cerr << "diagnostic_viewer: snapshot failed\n";
        return 1;
    }

    printJson(snapshot, scenarioStep);
    return 0;
}
