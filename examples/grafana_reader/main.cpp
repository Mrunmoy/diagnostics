#include "device.hpp"
#include "example_diag_tool.hpp"
#include "exporter.hpp"

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
    std::cerr << "usage: " << program << " [--demo|--prometheus|--json] [--scenario-step N]\n";
}

[[nodiscard]] bool parseStep(const char *text, std::uint32_t &outStep)
{
    if (text == nullptr || *text == '\0' || *text == '-')
    {
        return false;
    }

    char *end = nullptr;
    errno = 0;
    const unsigned long value = std::strtoul(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || value > UINT32_MAX)
    {
        return false;
    }

    outStep = static_cast<std::uint32_t>(value);
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    const char   *mode = "--demo";
    std::uint32_t scenarioStep = 0U;
    bool          useScenarioStep = false;

    if (argc != 1 && argc != 2 && argc != 4)
    {
        printUsage(argv[0]);
        return 1;
    }

    if (argc >= 2)
    {
        mode = argv[1];
    }

    if (argc == 4)
    {
        if (std::strcmp(argv[2], "--scenario-step") != 0 || !parseStep(argv[3], scenarioStep))
        {
            printUsage(argv[0]);
            return 1;
        }
        useScenarioStep = true;
    }

    if (std::strcmp(mode, "--demo") != 0 && std::strcmp(mode, "--prometheus") != 0 &&
        std::strcmp(mode, "--json") != 0)
    {
        printUsage(argv[0]);
        return 1;
    }

    grafana_reader::GrafanaDevice device{};
    if (device.setup() != diag::Result::Ok)
    {
        std::cerr << "grafana_reader: device setup failed\n";
        return 1;
    }

    if (useScenarioStep && device.applyScenario(scenarioStep) != diag::Result::Ok)
    {
        std::cerr << "grafana_reader: scenario step failed\n";
        return 1;
    }

    const example_diag::Device endpoint{"grafana_reader", &device.context(),
                                        grafana_reader::persistedSizeCallback, &device};
    example_diag::ToolSnapshot snapshot{};
    if (example_diag::collectSnapshot(endpoint, snapshot) != diag::Result::Ok)
    {
        std::cerr << "grafana_reader: tester snapshot failed\n";
        return 1;
    }

    diag::Result result = diag::Result::Ok;
    if (std::strcmp(mode, "--prometheus") == 0)
    {
        result = grafana_reader::exportPrometheus(std::cout, snapshot);
    }
    else if (std::strcmp(mode, "--json") == 0)
    {
        result = grafana_reader::exportJson(std::cout, snapshot);
    }
    else
    {
        std::cout << "grafana_reader: prometheus metrics\n";
        result = grafana_reader::exportPrometheus(std::cout, snapshot);
        if (result == diag::Result::Ok)
        {
            std::cout << "grafana_reader: json snapshot\n";
            result = grafana_reader::exportJson(std::cout, snapshot);
        }
    }

    if (result != diag::Result::Ok)
    {
        std::cerr << "grafana_reader: export failed\n";
        return 1;
    }

    return 0;
}
