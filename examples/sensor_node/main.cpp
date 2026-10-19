#include "diag/diag.hpp"
#include "example_diag_tool.hpp"

#include <iostream>

int main()
{
    diag::DtcRecord      dtcRecords[2]{};
    diag::ContextStorage storage{};
    diag::Config         config{dtcRecords, 2U};
    diag::Context        context{storage, config};

    const diag::Identity identity{diag::EcosystemId{1U},   diag::ProductId{10U},
                                  diag::DeviceType{1U},    diag::DeviceInstance{7U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{1U}};

    if (!context.isInitialized() || context.attachIdentity(identity) != diag::Result::Ok ||
        context.registerFault(diag::LocalFaultId{1U}, diag::DtcId{0x010001U},
                              diag::DtcSeverity::Warning) != diag::Result::Ok ||
        context.setFaultTestFailed(diag::LocalFaultId{1U}) != diag::Result::Ok)
    {
        std::cerr << "sensor_node: setup failed\n";
        return 1;
    }

    const diag::ResultValue<diag::DtcRecord> sensorFault = context.dtc(diag::DtcId{0x010001U});
    if (!sensorFault.hasValue() || !sensorFault.value().active)
    {
        std::cerr << "sensor_node: expected active sensor fault\n";
        return 1;
    }

    std::cout << "sensor_node: active runtime DTC 0x" << std::hex << sensorFault.value().id.value
              << std::dec << "\n";

    const example_diag::Device device{"sensor_node", &context, nullptr, nullptr};
    if (example_diag::runCli(device, diag::DtcId{0x010001U}, std::cout, std::cerr) !=
        diag::Result::Ok)
    {
        std::cerr << "sensor_node: diagnostic tool flow failed\n";
        return 1;
    }

    return 0;
}
