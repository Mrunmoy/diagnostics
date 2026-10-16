#include "diag/diag.hpp"

#include <cstdio>

int main()
{
    diag::DtcRecord      records[4]{};
    diag::ContextStorage storage{};
    diag::Config         config{records, 4U};
    diag::Context        diagnostics{storage, config};

    if (diagnostics.registerDtc(diag::DtcId{0x040101U}, diag::DtcSeverity::Critical) !=
        diag::Result::Ok)
    {
        std::fprintf(stderr, "dtc_example: failed to register DTC\n");
        return 1;
    }

    if (diagnostics.setDtcActive(diag::DtcId{0x040101U}, true) != diag::Result::Ok)
    {
        std::fprintf(stderr, "dtc_example: failed to set DTC active\n");
        return 2;
    }

    if (diagnostics.operationCycle() != diag::Result::Ok)
    {
        std::fprintf(stderr, "dtc_example: failed to advance operation cycle\n");
        return 3;
    }

    const diag::ResultValue<diag::DtcRecord> record = diagnostics.dtc(diag::DtcId{0x040101U});
    if (!record.hasValue())
    {
        std::fprintf(stderr, "dtc_example: failed to read DTC\n");
        return 4;
    }

    std::printf("dtc_example: id=0x%06x status=0x%02x active=%u occurrences=%u clear_count=%u\n",
                static_cast<unsigned>(record.value().id.value),
                static_cast<unsigned>(record.value().status), record.value().active ? 1U : 0U,
                static_cast<unsigned>(record.value().occurrenceCount),
                static_cast<unsigned>(record.value().clearCount));

    return 0;
}
