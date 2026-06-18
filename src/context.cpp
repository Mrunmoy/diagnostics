#include "diag/context.hpp"

#include <cstddef>
#include <new>

namespace diag
{

struct Context::State
{
    Config     config{};
    DirtyFlags dirtyFlags{0U};
    Identity   identity{};
    bool       initialized{false};
    bool       identityAttached{false};
};

struct Context::StorageLayout
{
    bool engaged{false};
    alignas(State) std::uint8_t state[sizeof(State)];
};

Context::Context(ContextStorage &storage, const Config &config) noexcept
{
    static_assert(sizeof(StorageLayout) <= ContextStorage::kSize,
                  "ContextStorage is too small for Context::StorageLayout");
    static_assert(alignof(StorageLayout) <= ContextStorage::kAlignment,
                  "ContextStorage alignment is too small for Context::StorageLayout");

    if (storage.bytes[0] != 0U)
    {
        return;
    }

    m_storage = new (static_cast<void *>(storage.bytes)) StorageLayout{};
    m_storage->engaged = true;

    void *const rawStorage = static_cast<void *>(m_storage->state);
    m_state = new (rawStorage) State{};
    m_state->config = config;
    m_state->initialized = true;
}

Context::~Context() noexcept
{
    if (m_state != nullptr && m_storage != nullptr)
    {
        m_state->~State();
        m_storage->engaged = false;
        m_storage = nullptr;
        m_state = nullptr;
    }
}

bool Context::isInitialized() const noexcept
{
    return (m_state != nullptr) && m_state->initialized;
}

DirtyFlags Context::dirtyFlags() const noexcept
{
    return isInitialized() ? m_state->dirtyFlags : 0U;
}

ResultValue<Identity> Context::identity() const noexcept
{
    if (!isInitialized())
    {
        return ResultValue<Identity>{Result::NotInitialized};
    }

    if (!m_state->identityAttached)
    {
        return ResultValue<Identity>{Result::NotFound};
    }

    return ResultValue<Identity>{m_state->identity};
}

Result Context::markDirty(const DirtyFlag flag) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->dirtyFlags |= static_cast<DirtyFlags>(flag);
    return Result::Ok;
}

Result Context::clearDirty(const DirtyFlag flag) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->dirtyFlags &= ~static_cast<DirtyFlags>(flag);
    return Result::Ok;
}

Result Context::attachIdentity(const Identity &identity) noexcept
{
    if (!isInitialized())
    {
        return Result::NotInitialized;
    }

    m_state->identity = identity;
    m_state->identityAttached = true;

    return Result::Ok;
}

} // namespace diag
