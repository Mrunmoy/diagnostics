#include "device.hpp"

#include <cstring>

namespace diagnostic_session
{
namespace
{

diag::Result sessionLoad(void *const user, std::uint8_t *const buffer, const std::size_t capacity,
                         std::size_t &bytesRead) noexcept
{
    SessionStorage &store = *static_cast<SessionStorage *>(user);
    if (capacity < store.used)
    {
        return diag::Result::Capacity;
    }

    std::memcpy(buffer, store.bytes, store.used);
    bytesRead = store.used;
    return diag::Result::Ok;
}

diag::Result sessionSave(void *const user, const std::uint8_t *const buffer,
                         const std::size_t size) noexcept
{
    SessionStorage &store = *static_cast<SessionStorage *>(user);
    if (size > sizeof(store.bytes))
    {
        return diag::Result::Capacity;
    }

    std::memcpy(store.bytes, buffer, size);
    store.used = size;
    return diag::Result::Ok;
}

diag::Result sessionClear(void *const user) noexcept
{
    SessionStorage &store = *static_cast<SessionStorage *>(user);
    store.used = 0U;
    return diag::Result::Ok;
}

} // namespace

DiagnosticDevice::DiagnosticDevice() noexcept = default;

diag::Result DiagnosticDevice::setup() noexcept
{
    const diag::Identity identity{diag::EcosystemId{7U},   diag::ProductId{42U},
                                  diag::DeviceType{3U},    diag::DeviceInstance{1U},
                                  diag::FirmwareStage{1U}, diag::FirmwareComponent{2U}};

    const diag::Storage storage{
        diag::StorageOps{sessionLoad, sessionSave, sessionClear},
        &m_persistentStore,
        diag::StorageCapabilities{0xFFU, 4U, diag::StorageAtomicCommit::Adapter,
                                  diag::StorageWearLeveling::Adapter},
        m_capsuleBuffer,
        sizeof(m_capsuleBuffer),
    };

    if (!m_context.isInitialized())
    {
        return diag::Result::NotInitialized;
    }

    diag::Result result = m_context.attachIdentity(identity);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = m_context.attachStorage(storage);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = m_context.registerFault(diag::LocalFaultId{11U}, diag::DtcId{0x030101U},
                                     diag::DtcSeverity::Error);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = m_context.registerFault(diag::LocalFaultId{12U}, diag::DtcId{0x030102U},
                                     diag::DtcSeverity::Warning);
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = m_context.setFaultTestFailed(diag::LocalFaultId{11U});
    if (result != diag::Result::Ok)
    {
        return result;
    }

    result = m_context.operationCycle();
    if (result != diag::Result::Ok)
    {
        return result;
    }

    return m_context.savePersistent();
}

diag::Context &DiagnosticDevice::context() noexcept
{
    return m_context;
}

std::size_t DiagnosticDevice::persistedBytes() const noexcept
{
    return m_persistentStore.used;
}

std::size_t persistedSizeCallback(void *const user) noexcept
{
    const DiagnosticDevice *const device = static_cast<const DiagnosticDevice *>(user);
    return (device == nullptr) ? 0U : device->persistedBytes();
}

} // namespace diagnostic_session
