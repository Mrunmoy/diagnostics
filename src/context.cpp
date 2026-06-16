#include "diag/context.hpp"

#include <cstddef>
#include <new>

namespace diag
{

struct Context::State
{
    Config     config{};
    DirtyFlags dirtyFlags{0U};
    bool       initialized{false};
};

Context::Context(ContextStorage &storage, const Config &config) noexcept
{
    static_assert(sizeof(State) <= ContextStorage::kSize,
                  "ContextStorage is too small for Context::State");
    static_assert(alignof(State) <= ContextStorage::kAlignment,
                  "ContextStorage alignment is too small for Context::State");

    void *const rawStorage = static_cast<void *>(storage.bytes);
    m_state = new (rawStorage) State{};
    m_state->config = config;
    m_state->initialized = true;
}

Context::~Context() noexcept
{
    if (m_state != nullptr)
    {
        m_state->~State();
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

} // namespace diag
