#include "device.hpp"

#include <cstring>

namespace diagnostic_viewer
{
namespace
{

diag::Result viewerLoad(void *const user, std::uint8_t *const buffer, const std::size_t bufferSize,
                        std::size_t &bytesRead) noexcept
{
    ViewerStore &store = *static_cast<ViewerStore *>(user);
    if (bufferSize < store.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, store.bytes, store.used);
    bytesRead = store.used;
    return diag::Result::Ok;
}

diag::Result viewerSave(void *const user, const std::uint8_t *const buffer,
                        const std::size_t size) noexcept
{
    ViewerStore &store = *static_cast<ViewerStore *>(user);
    if (size > sizeof(store.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(store.bytes, buffer, size);
    store.used = size;
    return diag::Result::Ok;
}

diag::Result viewerClearStore(void *const user) noexcept
{
    static_cast<ViewerStore *>(user)->used = 0U;
    return diag::Result::Ok;
}

} // namespace

diag::Result ViewerDevice::setup() noexcept
{
    const diag::Identity identity{diag::EcosystemId{7U},   diag::ProductId{120U},
                                  diag::DeviceType{8U},    diag::DeviceInstance{2U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{5U}};
    const diag::Storage  storage{
        diag::StorageOps{viewerLoad, viewerSave, viewerClearStore},
        &m_persistentStore,
        diag::StorageCapabilities{0xFFU, 8U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        m_capsuleBuffer,
        sizeof(m_capsuleBuffer),
    };

    if (!m_context.isInitialized() || m_context.attachIdentity(identity) != diag::Result::Ok ||
        m_context.attachStorage(storage) != diag::Result::Ok ||
        m_context.loadPersistent() != diag::Result::Ok ||
        m_context.registerFault(diag::LocalFaultId{31U}, diag::DtcId{0x050101U},
                                diag::DtcSeverity::Error) != diag::Result::Ok ||
        m_context.registerFault(diag::LocalFaultId{32U}, diag::DtcId{0x050102U},
                                diag::DtcSeverity::Warning) != diag::Result::Ok ||
        m_context.registerFault(diag::LocalFaultId{33U}, diag::DtcId{0x050103U},
                                diag::DtcSeverity::Info) != diag::Result::Ok ||
        m_context.setFaultTestFailed(diag::LocalFaultId{31U}) != diag::Result::Ok ||
        m_context.operationCycle() != diag::Result::Ok ||
        m_context.savePersistent() != diag::Result::Ok)
    {
        return diag::Result::NotInitialized;
    }

    return diag::Result::Ok;
}

diag::Result ViewerDevice::applyScenario(const std::uint32_t step) noexcept
{
    const std::uint32_t phase = step % 10U;
    diag::Result        result = diag::Result::Ok;

    if (phase >= 2U && phase <= 6U)
    {
        result = m_context.setFaultTestFailed(diag::LocalFaultId{32U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 3U || phase == 4U)
    {
        result = m_context.setFaultTestFailed(diag::LocalFaultId{33U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 4U || phase == 5U || phase == 6U)
    {
        result = m_context.setFaultTestPassed(diag::LocalFaultId{31U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    result = m_context.operationCycle();
    if (result != diag::Result::Ok)
    {
        return result;
    }

    if (phase == 5U)
    {
        result = m_context.setFaultTestPassed(diag::LocalFaultId{33U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
        result = m_context.clearDtc(diag::DtcId{0x050103U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 7U)
    {
        result = m_context.setFaultTestPassed(diag::LocalFaultId{32U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 8U)
    {
        result = m_context.clearDtc(diag::DtcId{0x050102U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    if (phase == 9U)
    {
        result = m_context.clearDtc(diag::DtcId{0x050101U});
        if (result != diag::Result::Ok)
        {
            return result;
        }
    }

    return m_context.savePersistent();
}

diag::Result ViewerDevice::clearDtc(const diag::DtcId dtcId) noexcept
{
    const diag::Result result = m_context.clearDtc(dtcId);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    return m_context.savePersistent();
}

diag::Context &ViewerDevice::context() noexcept
{
    return m_context;
}

std::size_t ViewerDevice::persistedBytes() const noexcept
{
    return m_persistentStore.used;
}

std::size_t persistedSizeCallback(void *const user) noexcept
{
    const ViewerDevice *const device = static_cast<const ViewerDevice *>(user);
    return (device == nullptr) ? 0U : device->persistedBytes();
}

} // namespace diagnostic_viewer
