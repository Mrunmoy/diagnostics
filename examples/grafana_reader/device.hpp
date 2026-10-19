#pragma once

#include "diag/diag.hpp"
#include "example_diag_tool.hpp"

#include <cstddef>
#include <cstdint>

namespace grafana_reader
{

struct GrafanaStore
{
    std::uint8_t bytes[384]{};
    std::size_t  used{0U};
};

class GrafanaDevice
{
  public:
    GrafanaDevice() noexcept = default;

    GrafanaDevice(const GrafanaDevice &) = delete;
    GrafanaDevice &operator=(const GrafanaDevice &) = delete;

    [[nodiscard]] diag::Result   setup() noexcept;
    [[nodiscard]] diag::Result   applyScenario(std::uint32_t step) noexcept;
    [[nodiscard]] diag::Context &context() noexcept;
    [[nodiscard]] std::size_t    persistedBytes() const noexcept;

  private:
    diag::ContextStorage m_contextStorage{};
    diag::DtcRecord      m_dtcRecords[4]{};
    GrafanaStore         m_persistentStore{};
    std::uint8_t         m_capsuleBuffer[384]{};
    diag::Config         m_config{m_dtcRecords, 4U, 1U, 10U};
    diag::Context        m_context{m_contextStorage, m_config};
};

[[nodiscard]] std::size_t persistedSizeCallback(void *user) noexcept;

} // namespace grafana_reader
