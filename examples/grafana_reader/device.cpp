#include "device.hpp"

#include <cstring>

namespace grafana_reader
{
namespace
{

diag::Result grafanaLoad(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                         std::size_t &bytesRead) noexcept
{
    GrafanaStore &store = *static_cast<GrafanaStore *>(user);
    if (bufferSize < store.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, store.bytes, store.used);
    bytesRead = store.used;
    return diag::Result::Ok;
}

diag::Result grafanaSave(void *const user, const std::uint8_t *const buffer,
                         const std::size_t size) noexcept
{
    GrafanaStore &store = *static_cast<GrafanaStore *>(user);
    if (size > sizeof(store.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(store.bytes, buffer, size);
    store.used = size;
    return diag::Result::Ok;
}

diag::Result grafanaClear(void *const user) noexcept
{
    static_cast<GrafanaStore *>(user)->used = 0U;
    return diag::Result::Ok;
}

} // namespace

diag::Result GrafanaDevice::setup() noexcept
{
    const diag::Identity identity{diag::EcosystemId{7U},   diag::ProductId{90U},
                                  diag::DeviceType{4U},    diag::DeviceInstance{3U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{1U}};
    const diag::Storage  storage{
        diag::StorageOps{grafanaLoad, grafanaSave, grafanaClear},
        &m_persistentStore,
        diag::StorageCapabilities{0xFFU, 4U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        m_capsuleBuffer,
        sizeof(m_capsuleBuffer),
    };

    if (!m_context.isInitialized() || m_context.attachIdentity(identity) != diag::Result::Ok ||
        m_context.attachStorage(storage) != diag::Result::Ok ||
        m_context.loadPersistent() != diag::Result::Ok ||
        m_context.registerFault(diag::LocalFaultId{21U}, diag::DtcId{0x040101U},
                                diag::DtcSeverity::Error) != diag::Result::Ok ||
        m_context.registerFault(diag::LocalFaultId{22U}, diag::DtcId{0x040102U},
                                diag::DtcSeverity::Warning) != diag::Result::Ok ||
        m_context.setFaultTestFailed(diag::LocalFaultId{21U}) != diag::Result::Ok ||
        m_context.operationCycle() != diag::Result::Ok ||
        m_context.savePersistent() != diag::Result::Ok)
    {
        return diag::Result::NotInitialized;
    }

    return diag::Result::Ok;
}

diag::Result GrafanaDevice::applyScenario(const std::uint32_t step) noexcept
{
    const std::uint32_t phase = step % 8U;
    diag::Result        result = diag::Result::Ok;

    if (phase >= 2U && phase <= 5U)
    {
        result = m_context.setFaultTestFailed(diag::LocalFaultId{22U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
        result = m_context.operationCycle();
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 4U || phase == 5U)
    {
        result = m_context.setFaultTestPassed(diag::LocalFaultId{21U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 5U)
    {
        result = m_context.setFaultTestPassed(diag::LocalFaultId{22U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
        result = m_context.clearDtc(diag::DtcId{0x040102U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 6U)
    {
        result = m_context.setFaultTestPassed(diag::LocalFaultId{21U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
        result = m_context.setFaultTestFailed(diag::LocalFaultId{21U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
        result = m_context.operationCycle();
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 7U)
    {
        result = m_context.clearDtc(diag::DtcId{0x040101U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    return m_context.savePersistent();
}

diag::Context &GrafanaDevice::context() noexcept
{
    return m_context;
}

std::size_t GrafanaDevice::persistedBytes() const noexcept
{
    return m_persistentStore.used;
}

std::size_t persistedSizeCallback(void *const user) noexcept
{
    const GrafanaDevice *const device = static_cast<const GrafanaDevice *>(user);
    return (device == nullptr) ? 0U : device->persistedBytes();
}

} // namespace grafana_reader
