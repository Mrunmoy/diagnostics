#pragma once

#include "diag/diag.hpp"
#include "example_diag_tool.hpp"

#include <cstddef>
#include <cstdint>

namespace diagnostic_viewer
{

struct ViewerStore
{
    std::uint8_t bytes[512]{};
    std::size_t  used{0U};
};

class ViewerDevice
{
  public:
    ViewerDevice() noexcept = default;

    ViewerDevice(const ViewerDevice &) = delete;
    ViewerDevice &operator=(const ViewerDevice &) = delete;

    [[nodiscard]] diag::Result   setup() noexcept;
    [[nodiscard]] diag::Result   applyScenario(std::uint32_t step) noexcept;
    [[nodiscard]] diag::Result   clearDtc(diag::DtcId dtcId) noexcept;
    [[nodiscard]] diag::Context &context() noexcept;
    [[nodiscard]] std::size_t    persistedBytes() const noexcept;

  private:
    diag::ContextStorage m_contextStorage{};
    diag::DtcRecord      m_dtcRecords[6]{};
    ViewerStore          m_persistentStore{};
    std::uint8_t         m_capsuleBuffer[512]{};
    diag::Config         m_config{m_dtcRecords, 6U, 1U, 6U};
    diag::Context        m_context{m_contextStorage, m_config};
};

[[nodiscard]] std::size_t persistedSizeCallback(void *user) noexcept;

} // namespace diagnostic_viewer
