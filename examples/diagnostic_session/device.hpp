#pragma once

#include "diag/diag.hpp"

#include <cstddef>
#include <cstdint>

namespace diagnostic_session
{

struct SessionStorage
{
    std::uint8_t bytes[384]{};
    std::size_t  used{0U};
};

class DiagnosticDevice
{
  public:
    DiagnosticDevice() noexcept;
    ~DiagnosticDevice() noexcept = default;

    DiagnosticDevice(const DiagnosticDevice &) = delete;
    DiagnosticDevice &operator=(const DiagnosticDevice &) = delete;

    [[nodiscard]] diag::Result   setup() noexcept;
    [[nodiscard]] diag::Context &context() noexcept;
    [[nodiscard]] std::size_t    persistedBytes() const noexcept;

  private:
    diag::ContextStorage m_contextStorage{};
    diag::DtcRecord      m_dtcRecords[4]{};
    SessionStorage       m_persistentStore{};
    std::uint8_t         m_capsuleBuffer[384]{};
    diag::Config         m_config{m_dtcRecords, 4U, 1U, 10U};
    diag::Context        m_context{m_contextStorage, m_config};
};

[[nodiscard]] std::size_t persistedSizeCallback(void *user) noexcept;

} // namespace diagnostic_session
